// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "VitruvioModule.h"

#include "PRTTypes.h"
#include "PRTUtils.h"
#include "UnrealCallbacks.h"
#include "UnrealResolveMapProvider.h"

#include "Util/AnnotationParsing.h"

#include "prt/API.h"
#include "prtx/EncoderInfoBuilder.h"

#include "Core.h"
#include "Interfaces/IPluginManager.h"
#include "MeshDescription.h"
#include "Modules/ModuleManager.h"
#include "StaticMeshAttributes.h"
#include "UObject/UObjectBaseUtility.h"

#define LOCTEXT_NAMESPACE "VitruvioModule"

DEFINE_LOG_CATEGORY(LogUnrealPrt);

namespace
{
	constexpr const wchar_t* ENC_ID_ATTR_EVAL = L"com.esri.prt.core.AttributeEvalEncoder";
	const FString DEFAULT_STYLE = L"Default";

	class FLoadResolveMapTask
	{
		FString ResolveMapUri;
		TPromise<ResolveMapSPtr> Promise;
		TMap<FString, ResolveMapSPtr>& ResolveMapCache;
		FCriticalSection& LoadResolveMapLock;

	public:
		FLoadResolveMapTask(TPromise<ResolveMapSPtr>&& InPromise, FString ResolveMapUri, TMap<FString, ResolveMapSPtr>& ResolveMapCache, FCriticalSection& LoadResolveMapLock)
			: ResolveMapUri(ResolveMapUri), Promise(MoveTemp(InPromise)), ResolveMapCache(ResolveMapCache), LoadResolveMapLock(LoadResolveMapLock)
		{
		}

		static const TCHAR* GetTaskName()
		{
			return TEXT("FLoadResolveMapTask");
		}
		FORCEINLINE static TStatId GetStatId()
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadResolveMapTask, STATGROUP_TaskGraphTasks);
		}

		static ENamedThreads::Type GetDesiredThread()
		{
			return ENamedThreads::AnyThread;
		}

		static ESubsequentsMode::Type GetSubsequentsMode()
		{
			return ESubsequentsMode::TrackSubsequents;
		}

		void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
		{
			const ResolveMapSPtr ResolveMap(prt::createResolveMap(*ResolveMapUri), PRTDestroyer());
			{
				FScopeLock Lock(&LoadResolveMapLock);
				ResolveMapCache.Add(ResolveMapUri, ResolveMap);
				Promise.SetValue(ResolveMap);
			}
		}
	};

	void SetInitialShapeGeometry(const InitialShapeBuilderUPtr& InitialShapeBuilder, const UStaticMesh* InitialShape)
	{
		check(InitialShape);

		if (!InitialShape->bAllowCPUAccess)
		{
			UE_LOG(LogUnrealPrt, Error, TEXT("Can not access static mesh geometry because bAllowCPUAccess is false"));
			return;
		}

		std::vector<double> vertexCoords;
		std::vector<uint32_t> indices;
		std::vector<uint32_t> faceCounts;
		if (InitialShape->RenderData != nullptr && InitialShape->RenderData->LODResources.IsValidIndex(0))
		{
			const FStaticMeshLODResources& LOD = InitialShape->RenderData->LODResources[0];

			for (auto SectionIndex = 0; SectionIndex < LOD.Sections.Num(); ++SectionIndex)
			{
				for (uint32 VertexIndex = 0; VertexIndex < LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++VertexIndex)
				{
					FVector Vertex = LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex);

					vertexCoords.push_back(Vertex[0] / 100);
					vertexCoords.push_back(Vertex[2] / 100);
					vertexCoords.push_back(Vertex[1] / 100);
				}

				const FStaticMeshSection& Section = LOD.Sections[SectionIndex];
				FIndexArrayView Indices = LOD.IndexBuffer.GetArrayView();

				for (uint32 Triangle = 0; Triangle < Section.NumTriangles; ++Triangle)
				{
					for (uint32 TriangleVertexIndex = 0; TriangleVertexIndex < 3; ++TriangleVertexIndex)
					{
						const uint32 MeshVertIndex = Indices[Section.FirstIndex + Triangle * 3 + TriangleVertexIndex];
						indices.push_back(MeshVertIndex);
					}

					faceCounts.push_back(3);
				}
			}
		}

		const prt::Status SetGeometryStatus =
			InitialShapeBuilder->setGeometry(vertexCoords.data(), vertexCoords.size(), indices.data(), indices.size(), faceCounts.data(), faceCounts.size());

		if (SetGeometryStatus != prt::STATUS_OK)
		{
			UE_LOG(LogUnrealPrt, Error, TEXT("InitialShapeBuilder setGeometry failed status = %hs"), prt::getStatusDescription(SetGeometryStatus))
		}
	}

	AttributeMapUPtr CreateAttributeMap(const TMap<FString, URuleAttribute*>& Attributes)
	{
		AttributeMapBuilderUPtr AttributeMapBuilder(prt::AttributeMapBuilder::create());

		for (const TPair<FString, URuleAttribute*>& AttributeEntry : Attributes)
		{
			const URuleAttribute* Attribute = AttributeEntry.Value;

			if (const UFloatAttribute* FloatAttribute = Cast<UFloatAttribute>(Attribute))
			{
				AttributeMapBuilder->setFloat(*Attribute->Name, FloatAttribute->Value);
			}
			else if (const UStringAttribute* StringAttribute = Cast<UStringAttribute>(Attribute))
			{
				AttributeMapBuilder->setString(*Attribute->Name, *StringAttribute->Value);
			}
			else if (const UBoolAttribute* BoolAttribute = Cast<UBoolAttribute>(Attribute))
			{
				AttributeMapBuilder->setBool(*Attribute->Name, BoolAttribute->Value);
			}
		}

		return AttributeMapUPtr(AttributeMapBuilder->createAttributeMap(), PRTDestroyer());
	}

	AttributeMapUPtr GetDefaultAttributeValues(const std::wstring& RuleFile, const std::wstring& StartRule, const ResolveMapSPtr& ResolveMapPtr, const UStaticMesh* InitialShape)
	{
		AttributeMapBuilderUPtr UnrealCallbacksAttributeBuilder(prt::AttributeMapBuilder::create());
		UnrealCallbacks UnrealCallbacks(UnrealCallbacksAttributeBuilder, nullptr, nullptr, nullptr);

		InitialShapeBuilderUPtr InitialShapeBuilder(prt::InitialShapeBuilder::create());

		SetInitialShapeGeometry(InitialShapeBuilder, InitialShape);

		// TODO calculate random seed
		const int32_t RandomSeed = 0;
		const AttributeMapUPtr EmptyAttributes(AttributeMapBuilderUPtr(prt::AttributeMapBuilder::create())->createAttributeMap());
		InitialShapeBuilder->setAttributes(RuleFile.c_str(), StartRule.c_str(), RandomSeed, L"", EmptyAttributes.get(), ResolveMapPtr.get());

		const InitialShapeUPtr Shape(InitialShapeBuilder->createInitialShapeAndReset());
		const InitialShapeNOPtrVector InitialShapes = {Shape.get()};

		const std::vector<const wchar_t*> EncoderIds = {ENC_ID_ATTR_EVAL};
		const AttributeMapUPtr AttributeEncodeOptions = prtu::createValidatedOptions(ENC_ID_ATTR_EVAL);
		const AttributeMapNOPtrVector EncoderOptions = {AttributeEncodeOptions.get()};

		prt::generate(InitialShapes.data(), InitialShapes.size(), nullptr, EncoderIds.data(), EncoderIds.size(), EncoderOptions.data(), &UnrealCallbacks, nullptr, nullptr);

		return AttributeMapUPtr(UnrealCallbacksAttributeBuilder->createAttributeMap());
	}

	URuleAttribute* CreateAttribute(const AttributeMapUPtr& AttributeMap, const prt::RuleFileInfo::Entry* AttrInfo)
	{
		const std::wstring Name(AttrInfo->getName());
		switch (AttrInfo->getReturnType())
		{
		case prt::AAT_BOOL:
		{
			UBoolAttribute* BoolAttribute = NewObject<UBoolAttribute>();
			BoolAttribute->Value = AttributeMap->getBool(Name.c_str());
			return BoolAttribute;
		}
		case prt::AAT_INT:
		case prt::AAT_FLOAT:
		{
			UFloatAttribute* FloatAttribute = NewObject<UFloatAttribute>();
			FloatAttribute->Value = AttributeMap->getFloat(Name.c_str());
			return FloatAttribute;
		}
		case prt::AAT_STR:
		{
			UStringAttribute* StringAttribute = NewObject<UStringAttribute>();
			StringAttribute->Value = AttributeMap->getString(Name.c_str());
			return StringAttribute;
		}
		case prt::AAT_UNKNOWN:
		case prt::AAT_VOID:
		case prt::AAT_BOOL_ARRAY:
		case prt::AAT_FLOAT_ARRAY:
		case prt::AAT_STR_ARRAY:
		default:
			return nullptr;
		}
	}

	TMap<FString, URuleAttribute*> ConvertAttributeMap(const AttributeMapUPtr& AttributeMap, const RuleFileInfoUPtr& RuleInfo)
	{
		TMap<FString, URuleAttribute*> Attributes;
		for (size_t AttributeIndex = 0; AttributeIndex < RuleInfo->getNumAttributes(); AttributeIndex++)
		{
			const prt::RuleFileInfo::Entry* AttrInfo = RuleInfo->getAttribute(AttributeIndex);
			if (AttrInfo->getNumParameters() != 0)
			{
				continue;
			}

			// We only support the default style for the moment
			FString Style(prtu::getStyle(AttrInfo->getName()).c_str());
			if (Style != DEFAULT_STYLE)
			{
				continue;
			}

			const std::wstring Name(AttrInfo->getName());
			if (Attributes.Contains(Name.c_str()))
			{
				continue;
			}

			URuleAttribute* Attribute = CreateAttribute(AttributeMap, AttrInfo);

			if (Attribute)
			{
				FString AttributeName = Name.c_str();
				FString DisplayName = prtu::removeImport(prtu::removeStyle(Name.c_str())).c_str();
				Attribute->Name = AttributeName;
				Attribute->DisplayName = DisplayName;

				ParseAttributeAnnotations(AttrInfo, *Attribute);

				if (!Attribute->Hidden)
				{
					Attributes.Add(AttributeName, Attribute);
				}
			}
		}
		return Attributes;
	}

	void CleanupGeometryEncoderDlls(const FString& BinariesPath)
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Find all Unreal dlls
		TArray<FString> OutFiles;
		PlatformFile.FindFiles(OutFiles, *BinariesPath, L".dll");
		TArray<FString> UnrealDlls = OutFiles.FilterByPredicate([](const FString& File) -> auto { return FPaths::GetCleanFilename(File).StartsWith(L"UE4Editor-"); });

		// Sort by date and remove newest (don't want to delete)
		UnrealDlls.Sort([&PlatformFile](const auto& A, const auto& B) -> auto { return PlatformFile.GetTimeStamp(*A) > PlatformFile.GetTimeStamp(*B); });
		if (UnrealDlls.Num() > 0)
		{
			UnrealDlls.RemoveAt(0);
		}

		// Delete old dlls
		for (const auto& OldDll : UnrealDlls)
		{
			PlatformFile.DeleteFile(*OldDll);
			FString PdbFile = FPaths::GetBaseFilename(OldDll, false) + L".pdb";
			if (PlatformFile.FileExists(*PdbFile))
			{
				PlatformFile.DeleteFile(*PdbFile);
			}
		}
	}

} // namespace

void VitruvioModule::StartupModule()
{
	const FString BaseDir = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin("UnrealGeometryEncoder")->GetBaseDir());
	const FString BinariesPath = FPaths::Combine(*BaseDir, L"Binaries", L"Win64");
	const FString PrtLibPath = FPaths::Combine(*BinariesPath, L"com.esri.prt.core.dll");

	PrtDllHandle = FPlatformProcess::GetDllHandle(*PrtLibPath);

	TArray<wchar_t*> PRTPluginsPaths;
	PRTPluginsPaths.Add(const_cast<wchar_t*>(*BinariesPath));

	// TODO this cleanup should happen in a post build step. See DatasmithExporter Plugin from the Unreal source for more information
	CleanupGeometryEncoderDlls(BinariesPath);

	LogHandler = new UnrealLogHandler;
	prt::addLogHandler(LogHandler);

	prt::Status Status;
	PrtLibrary = prt::init(PRTPluginsPaths.GetData(), PRTPluginsPaths.Num(), prt::LogLevel::LOG_TRACE, &Status);
	Initialized = Status == prt::STATUS_OK;

	PrtCache.reset(prt::CacheObject::create(prt::CacheObject::CACHE_TYPE_DEFAULT));
}

void VitruvioModule::ShutdownModule()
{
	if (PrtDllHandle)
	{
		FPlatformProcess::FreeDllHandle(PrtDllHandle);
		PrtDllHandle = nullptr;
	}
	if (PrtLibrary)
	{
		PrtLibrary->destroy();
	}

	delete LogHandler;
}

TFuture<ResolveMapSPtr> VitruvioModule::LoadResolveMapAsync(const std::wstring& InUri) const
{
	TPromise<ResolveMapSPtr> Promise;
	TFuture<ResolveMapSPtr> Future = Promise.GetFuture();
	FString Uri = InUri.c_str();

	// Check if has already been cached
	{
		FScopeLock Lock(&LoadResolveMapLock);
		const auto CachedResolveMap = ResolveMapCache.Find(Uri);
		if (CachedResolveMap)
		{
			Promise.SetValue(*CachedResolveMap);
			return Future;
		}
	}

	// Check if a task is already running for loading the specified resolve map
	FGraphEventRef* ScheduledTaskEvent;
	{
		FScopeLock Lock(&LoadResolveMapLock);
		ScheduledTaskEvent = ResolveMapEventGraphRefCache.Find(Uri);
	}
	if (ScheduledTaskEvent)
	{
		// Add task which only fetches the result from the cache once the actual loading has finished
		FGraphEventArray Prerequisites;
		Prerequisites.Add(*ScheduledTaskEvent);
		TGraphTask<TAsyncGraphTask<ResolveMapSPtr>>::CreateTask(&Prerequisites)
			.ConstructAndDispatchWhenReady(
				[this, Uri]() {
					FScopeLock Lock(&LoadResolveMapLock);
					return ResolveMapCache[Uri];
				},
				MoveTemp(Promise), ENamedThreads::AnyThread);
	}
	else
	{
		FGraphEventRef LoadTask;
		{
			FScopeLock Lock(&LoadResolveMapLock);
			// Task which does the actual resolve map loading which might take a long time
			LoadTask = TGraphTask<FLoadResolveMapTask>::CreateTask().ConstructAndDispatchWhenReady(MoveTemp(Promise), Uri, ResolveMapCache, LoadResolveMapLock);
			ResolveMapEventGraphRefCache.Add(Uri, LoadTask);
		}

		// Task which removes the event from the cache once finished
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[this, Uri]() {
				FScopeLock Lock(&LoadResolveMapLock);
				ResolveMapEventGraphRefCache.Remove(Uri);
			},
			TStatId(), LoadTask, ENamedThreads::AnyThread);
	}

	return Future;
}

FGenerateResult VitruvioModule::Generate(const UStaticMesh* InitialShape, UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent, URulePackage* RulePackage,
										 const TMap<FString, URuleAttribute*>& Attributes) const
{
	check(InitialShape);
	check(RulePackage);

	if (!Initialized)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("prt not initialized"))
		return {};
	}

	const InitialShapeBuilderUPtr InitialShapeBuilder(prt::InitialShapeBuilder::create());
	SetInitialShapeGeometry(InitialShapeBuilder, InitialShape);

	const FString PathName = RulePackage->GetPathName();
	const std::wstring PathUri = UnrealResolveMapProvider::SCHEME_UNREAL + L":" + *PathName;
	const ResolveMapSPtr ResolveMap = LoadResolveMapAsync(PathUri).Get();

	const std::wstring RuleFile = prtu::getRuleFileEntry(ResolveMap);
	const wchar_t* RuleFileUri = ResolveMap->getString(RuleFile.c_str());

	const RuleFileInfoUPtr StartRuleInfo(prt::createRuleFileInfo(RuleFileUri));
	const std::wstring StartRule = prtu::detectStartRule(StartRuleInfo);

	// TODO calculate random seed
	const int32_t RandomSeed = 0;
	const AttributeMapUPtr AttributeMap = CreateAttributeMap(Attributes);
	InitialShapeBuilder->setAttributes(RuleFile.c_str(), StartRule.c_str(), RandomSeed, L"", AttributeMap.get(), ResolveMap.get());

	AttributeMapBuilderUPtr AttributeMapBuilder(prt::AttributeMapBuilder::create());
	const std::unique_ptr<UnrealCallbacks> OutputHandler(new UnrealCallbacks(AttributeMapBuilder, OpaqueParent, MaskedParent, TranslucentParent));

	const InitialShapeUPtr Shape(InitialShapeBuilder->createInitialShapeAndReset());

	const std::vector<const wchar_t*> EncoderIds = {ENCODER_ID_UnrealGeometry};
	const AttributeMapUPtr UnrealEncoderOptions(prtu::createValidatedOptions(ENCODER_ID_UnrealGeometry));
	const AttributeMapNOPtrVector EncoderOptions = {UnrealEncoderOptions.get()};

	InitialShapeNOPtrVector Shapes = {Shape.get()};

	const prt::Status GenerateStatus =
		prt::generate(Shapes.data(), Shapes.size(), nullptr, EncoderIds.data(), EncoderIds.size(), EncoderOptions.data(), OutputHandler.get(), PrtCache.get(), nullptr);

	if (GenerateStatus != prt::STATUS_OK)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("prt generate failed: %hs"), prt::getStatusDescription(GenerateStatus))
	}

	return {OutputHandler->GetModel(), OutputHandler->GetInstances()};
}

TFuture<FGenerateResult> VitruvioModule::GenerateAsync(const UStaticMesh* InitialShape, UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent,
													   URulePackage* RulePackage, const TMap<FString, URuleAttribute*>& Attributes) const
{
	check(InitialShape);
	check(RulePackage);

	if (!Initialized)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("prt not initialized"))
		return {};
	}

	return Async(EAsyncExecution::Thread, [=]() -> FGenerateResult { return Generate(InitialShape, OpaqueParent, MaskedParent, TranslucentParent, RulePackage, Attributes); });
}

TFuture<TMap<FString, URuleAttribute*>> VitruvioModule::LoadDefaultRuleAttributesAsync(const UStaticMesh* InitialShape, URulePackage* RulePackage) const
{
	check(InitialShape);
	check(RulePackage);

	return Async(EAsyncExecution::Thread, [=]() -> TMap<FString, URuleAttribute*> {
		const FString PathName = RulePackage->GetPathName();
		const std::wstring PathUri = UnrealResolveMapProvider::SCHEME_UNREAL + L":" + *PathName;
		const ResolveMapSPtr ResolveMap = LoadResolveMapAsync(PathUri).Get();

		const std::wstring RuleFile = prtu::getRuleFileEntry(ResolveMap);
		const wchar_t* RuleFileUri = ResolveMap->getString(RuleFile.c_str());

		const RuleFileInfoUPtr StartRuleInfo(prt::createRuleFileInfo(RuleFileUri));
		const std::wstring StartRule = prtu::detectStartRule(StartRuleInfo);

		prt::Status InfoStatus;
		const RuleFileInfoUPtr RuleInfo(prt::createRuleFileInfo(RuleFileUri, nullptr, &InfoStatus));
		if (!RuleInfo || InfoStatus != prt::STATUS_OK)
		{
			UE_LOG(LogUnrealPrt, Error, TEXT("could not get rule file info from rule file %s"), RuleFileUri)
			return {};
		}

		const AttributeMapUPtr DefaultAttributeMap(GetDefaultAttributeValues(RuleFile.c_str(), StartRule.c_str(), ResolveMap, InitialShape));
		return ConvertAttributeMap(DefaultAttributeMap, RuleInfo);
	});
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(VitruvioModule, Vitruvio)
