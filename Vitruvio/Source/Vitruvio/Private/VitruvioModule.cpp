// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "VitruvioModule.h"

#include "PRTTypes.h"
#include "PRTUtils.h"
#include "UnrealCallbacks.h"
#include "UnrealResolveMapProvider.h"

#include "Util/AnnotationParsing.h"
#include "Util/PolygonWindings.h"

#include "prt/API.h"
#include "prtx/EncoderInfoBuilder.h"

#include "ThirdParty/zlib/zlib-1.2.5/Src/contrib/minizip/unzip.h"

#include "Core.h"
#include "HttpModule.h"
#include "Interfaces/IPluginManager.h"
#include "MeshDescription.h"
#include "Modules/ModuleManager.h"
#include "Online/HTTP/Public/Interfaces/IHttpRequest.h"
#include "Online/HTTP/Public/Interfaces/IHttpResponse.h"
#include "UObject/UObjectBaseUtility.h"
#include "Util/AttributeConversion.h"

#define LOCTEXT_NAMESPACE "VitruvioModule"

DEFINE_LOG_CATEGORY(LogUnrealPrt);

namespace
{
constexpr const wchar_t* ATTRIBUTE_EVAL_ENCODER_ID = L"com.esri.prt.core.AttributeEvalEncoder";

class FLoadResolveMapTask
{
	TLazyObjectPtr<URulePackage> LazyRulePackagePtr;
	FString ResolveMapUri;
	TPromise<ResolveMapSPtr> Promise;
	TMap<TLazyObjectPtr<URulePackage>, ResolveMapSPtr>& ResolveMapCache;
	FCriticalSection& LoadResolveMapLock;

public:
	FLoadResolveMapTask(TPromise<ResolveMapSPtr>&& InPromise, const TLazyObjectPtr<URulePackage> LazyRulePackagePtr, const FString ResolveMapUri,
						TMap<TLazyObjectPtr<URulePackage>, ResolveMapSPtr>& ResolveMapCache, FCriticalSection& LoadResolveMapLock)
		: LazyRulePackagePtr(LazyRulePackagePtr), ResolveMapUri(ResolveMapUri), Promise(MoveTemp(InPromise)), ResolveMapCache(ResolveMapCache),
		  LoadResolveMapLock(LoadResolveMapLock)
	{
	}

	static const TCHAR* GetTaskName() { return TEXT("FLoadResolveMapTask"); }
	FORCEINLINE static TStatId GetStatId() { RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadResolveMapTask, STATGROUP_TaskGraphTasks); }

	static ENamedThreads::Type GetDesiredThread() { return ENamedThreads::AnyThread; }

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		const ResolveMapSPtr ResolveMap(prt::createResolveMap(TCHAR_TO_WCHAR(*ResolveMapUri)), PRTDestroyer());
		{
			FScopeLock Lock(&LoadResolveMapLock);
			ResolveMapCache.Add(LazyRulePackagePtr, ResolveMap);
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

	TArray<FVector> MeshVertices;
	TArray<int32> MeshIndices;

	if (InitialShape->RenderData != nullptr && InitialShape->RenderData->LODResources.IsValidIndex(0))
	{
		const FStaticMeshLODResources& LOD = InitialShape->RenderData->LODResources[0];

		for (auto SectionIndex = 0; SectionIndex < LOD.Sections.Num(); ++SectionIndex)
		{
			for (uint32 VertexIndex = 0; VertexIndex < LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++VertexIndex)
			{
				FVector Vertex = LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex);
				MeshVertices.Add(Vertex);
			}

			const FStaticMeshSection& Section = LOD.Sections[SectionIndex];
			FIndexArrayView IndicesView = LOD.IndexBuffer.GetArrayView();

			for (uint32 Triangle = 0; Triangle < Section.NumTriangles; ++Triangle)
			{
				for (uint32 TriangleVertexIndex = 0; TriangleVertexIndex < 3; ++TriangleVertexIndex)
				{
					const uint32 MeshVertIndex = IndicesView[Section.FirstIndex + Triangle * 3 + TriangleVertexIndex];
					MeshIndices.Add(MeshVertIndex);
				}
			}
		}
	}

	const TArray<TArray<FVector>> Windings = Vitruvio::PolygonWindings::GetOutsideWindings(MeshVertices, MeshIndices);

	std::vector<double> vertexCoords;
	std::vector<uint32_t> indices;
	std::vector<uint32_t> faceCounts;

	uint32_t CurrentIndex = 0;
	for (const TArray<FVector>& Winding : Windings)
	{
		faceCounts.push_back(Winding.Num());
		for (const FVector& Vertex : Winding)
		{
			indices.push_back(CurrentIndex++);

			const FVector CEVertex = FVector(Vertex.X, Vertex.Z, Vertex.Y) / 100.0;
			vertexCoords.push_back(CEVertex.X);
			vertexCoords.push_back(CEVertex.Y);
			vertexCoords.push_back(CEVertex.Z);
		}
	}

	const prt::Status SetGeometryStatus = InitialShapeBuilder->setGeometry(vertexCoords.data(), vertexCoords.size(), indices.data(), indices.size(),
																		   faceCounts.data(), faceCounts.size());

	if (SetGeometryStatus != prt::STATUS_OK)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("InitialShapeBuilder setGeometry failed status = %hs"), prt::getStatusDescription(SetGeometryStatus))
	}
}

AttributeMapUPtr GetDefaultAttributeValues(const std::wstring& RuleFile, const std::wstring& StartRule, const ResolveMapSPtr& ResolveMapPtr,
										   const UStaticMesh* InitialShape, const int32 RandomSeed)
{
	AttributeMapBuilderUPtr UnrealCallbacksAttributeBuilder(prt::AttributeMapBuilder::create());
	UnrealCallbacks UnrealCallbacks(UnrealCallbacksAttributeBuilder, nullptr, nullptr, nullptr);

	InitialShapeBuilderUPtr InitialShapeBuilder(prt::InitialShapeBuilder::create());

	SetInitialShapeGeometry(InitialShapeBuilder, InitialShape);

	const AttributeMapUPtr EmptyAttributes(AttributeMapBuilderUPtr(prt::AttributeMapBuilder::create())->createAttributeMap());
	InitialShapeBuilder->setAttributes(RuleFile.c_str(), StartRule.c_str(), RandomSeed, L"", EmptyAttributes.get(), ResolveMapPtr.get());

	const InitialShapeUPtr Shape(InitialShapeBuilder->createInitialShapeAndReset());
	const InitialShapeNOPtrVector InitialShapes = {Shape.get()};

	const std::vector<const wchar_t*> EncoderIds = {ATTRIBUTE_EVAL_ENCODER_ID};
	const AttributeMapUPtr AttributeEncodeOptions = prtu::createValidatedOptions(ATTRIBUTE_EVAL_ENCODER_ID);
	const AttributeMapNOPtrVector EncoderOptions = {AttributeEncodeOptions.get()};

	prt::generate(InitialShapes.data(), InitialShapes.size(), nullptr, EncoderIds.data(), EncoderIds.size(), EncoderOptions.data(), &UnrealCallbacks,
				  nullptr, nullptr);

	return AttributeMapUPtr(UnrealCallbacksAttributeBuilder->createAttributeMap());
}

FString GetBinariesPath()
{
	const FString BaseDir = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin("UnrealGeometryEncoder")->GetBaseDir());
	const FString BinariesPath = FPaths::Combine(*BaseDir, TEXT("Binaries"), TEXT("Win64"));
	return BinariesPath;
}

FString GetPrtDllPath()
{
	return FPaths::Combine(*GetBinariesPath(), TEXT("com.esri.prt.core.dll"));
}

bool PrtInstalled()
{
	return FPlatformFileManager::Get().GetPlatformFile().FileExists(*GetPrtDllPath());
}

int32 Unzip(const FString& ZipPath)
{
	static const int32 BUFFER_SIZE = 8096;
	static const int32 MAX_FILE_LENGTH = 512;

	const FString ZipFolder = FPaths::GetPath(ZipPath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	unzFile ZipFile = unzOpen(TCHAR_TO_ANSI(*ZipPath));

	unz_global_info UnzGlobalInfo;
	if (unzGetGlobalInfo(ZipFile, &UnzGlobalInfo) != UNZ_OK)
	{
		unzClose(ZipFile);
		return -1;
	}

	uint8 ReadBuffer[BUFFER_SIZE];
	for (unsigned long FileIndex = 0; FileIndex < UnzGlobalInfo.number_entry; ++FileIndex)
	{
		unz_file_info FileInfo;
		char Filename[MAX_FILE_LENGTH];
		if (unzGetCurrentFileInfo(ZipFile, &FileInfo, Filename, MAX_FILE_LENGTH, nullptr, 0, nullptr, 0) != UNZ_OK)
		{
			unzClose(ZipFile);
			return -1;
		}

		const FString FullPath = FPaths::Combine(ZipFolder, ANSI_TO_TCHAR(Filename));
		PlatformFile.CreateDirectoryTree(*FPaths::GetPath(FullPath));

		IFileHandle* Handle = PlatformFile.OpenWrite(*FullPath, false, false);
		if (unzOpenCurrentFile(ZipFile) != UNZ_OK)
		{
			delete Handle;
			unzClose(ZipFile);
			return -1;
		}

		int Read;
		while ((Read = unzReadCurrentFile(ZipFile, ReadBuffer, BUFFER_SIZE)) > 0)
		{
			Handle->Write(ReadBuffer, Read);
		}

		if (Read != UNZ_OK)
		{
			return -1;
		}

		if (unzGoToNextFile(ZipFile) != UNZ_OK)
		{
			unzClose(ZipFile);
			return -1;
		}

		unzCloseCurrentFile(ZipFile);
		Handle->Flush();
		delete Handle;
	}

	return 0;
}

} // namespace

void VitruvioModule::DownloadPrt()
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("GET"));
	Request->SetURL("https://github.com/Esri/cityengine-sdk/releases/download/2.2.6332/esri_ce_sdk-2.2.6332-win10-vc142-x86_64-rel-opt.zip");
	Request->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded) {
		const FString BinariesFolder = GetBinariesPath();
		const FString PrtZip = FPaths::Combine(BinariesFolder, TEXT("esri_ce_sdk-2.2.6332-win10-vc142-x86_64-rel-opt.zip"));

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		IFileHandle* Handle = PlatformFile.OpenWrite(*PrtZip, false, false);
		const TArray<uint8>& ZipContent = HttpResponse->GetContent();
		Handle->Write(ZipContent.GetData(), ZipContent.Num());
		delete Handle;
	});
	Request->ProcessRequest();
}

void VitruvioModule::InitializePrt()
{
	const FString PrtLibPath = GetPrtDllPath();
	PrtDllHandle = FPlatformProcess::GetDllHandle(*PrtLibPath);

	TArray<wchar_t*> PRTPluginsPaths;
	PRTPluginsPaths.Add(const_cast<wchar_t*>(TCHAR_TO_WCHAR(*GetBinariesPath())));

	LogHandler = new UnrealLogHandler;
	prt::addLogHandler(LogHandler);

	prt::Status Status;
	PrtLibrary = prt::init(PRTPluginsPaths.GetData(), PRTPluginsPaths.Num(), prt::LogLevel::LOG_TRACE, &Status);
	Initialized = Status == prt::STATUS_OK;

	PrtCache.reset(prt::CacheObject::create(prt::CacheObject::CACHE_TYPE_DEFAULT));
}

void VitruvioModule::StartupModule()
{
	InitializePrt();
}

void VitruvioModule::ShutdownModule()
{
	// TODO: what happens if we still have threads busy with generation?

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

TFuture<FGenerateResult> VitruvioModule::GenerateAsync(const UStaticMesh* InitialShape, UMaterial* OpaqueParent, UMaterial* MaskedParent,
													   UMaterial* TranslucentParent, URulePackage* RulePackage,
													   const TMap<FString, URuleAttribute*>& Attributes, const int32 RandomSeed) const
{
	check(InitialShape);
	check(RulePackage);

	if (!Initialized)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("prt not initialized"))
		return {};
	}

	return Async(EAsyncExecution::Thread, [=]() -> FGenerateResult {
		return Generate(InitialShape, OpaqueParent, MaskedParent, TranslucentParent, RulePackage, Attributes, RandomSeed);
	});
}

FGenerateResult VitruvioModule::Generate(const UStaticMesh* InitialShape, UMaterial* OpaqueParent, UMaterial* MaskedParent,
										 UMaterial* TranslucentParent, URulePackage* RulePackage, const TMap<FString, URuleAttribute*>& Attributes,
										 const int32 RandomSeed) const
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

	const ResolveMapSPtr ResolveMap = LoadResolveMapAsync(RulePackage).Get();

	const std::wstring RuleFile = prtu::getRuleFileEntry(ResolveMap);
	const wchar_t* RuleFileUri = ResolveMap->getString(RuleFile.c_str());

	const RuleFileInfoUPtr StartRuleInfo(prt::createRuleFileInfo(RuleFileUri));
	const std::wstring StartRule = prtu::detectStartRule(StartRuleInfo);

	const AttributeMapUPtr AttributeMap = Vitruvio::AttributeConversion::CreateAttributeMap(Attributes);
	InitialShapeBuilder->setAttributes(RuleFile.c_str(), StartRule.c_str(), RandomSeed, L"", AttributeMap.get(), ResolveMap.get());

	AttributeMapBuilderUPtr AttributeMapBuilder(prt::AttributeMapBuilder::create());
	const std::unique_ptr<UnrealCallbacks> OutputHandler(new UnrealCallbacks(AttributeMapBuilder, OpaqueParent, MaskedParent, TranslucentParent));

	const InitialShapeUPtr Shape(InitialShapeBuilder->createInitialShapeAndReset());

	const std::vector<const wchar_t*> EncoderIds = {UNREAL_GEOMETRY_ENCODER_ID};
	const AttributeMapUPtr UnrealEncoderOptions(prtu::createValidatedOptions(UNREAL_GEOMETRY_ENCODER_ID));
	const AttributeMapNOPtrVector EncoderOptions = {UnrealEncoderOptions.get()};

	InitialShapeNOPtrVector Shapes = {Shape.get()};

	const prt::Status GenerateStatus = prt::generate(Shapes.data(), Shapes.size(), nullptr, EncoderIds.data(), EncoderIds.size(),
													 EncoderOptions.data(), OutputHandler.get(), PrtCache.get(), nullptr);

	if (GenerateStatus != prt::STATUS_OK)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("prt generate failed: %hs"), prt::getStatusDescription(GenerateStatus))
	}

	return {OutputHandler->GetModel(), OutputHandler->GetInstances()};
}

TFuture<FAttributeMap> VitruvioModule::LoadDefaultRuleAttributesAsync(const UStaticMesh* InitialShape, URulePackage* RulePackage,
																	  const int32 RandomSeed) const
{
	check(InitialShape);
	check(RulePackage);

	if (!Initialized)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("prt not initialized"))
		return {};
	}

	return Async(EAsyncExecution::Thread, [=]() -> FAttributeMap {
		const ResolveMapSPtr ResolveMap = LoadResolveMapAsync(RulePackage).Get();

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

		const AttributeMapUPtr DefaultAttributeMap(
			GetDefaultAttributeValues(RuleFile.c_str(), StartRule.c_str(), ResolveMap, InitialShape, RandomSeed));
		return Vitruvio::AttributeConversion::ConvertAttributeMap(DefaultAttributeMap, RuleInfo);
	});
}

TFuture<ResolveMapSPtr> VitruvioModule::LoadResolveMapAsync(URulePackage* const RulePackage) const
{
	TPromise<ResolveMapSPtr> Promise;
	TFuture<ResolveMapSPtr> Future = Promise.GetFuture();

	const TLazyObjectPtr<URulePackage> LazyRulePackagePtr(RulePackage);

	// Check if has already been cached
	{
		FScopeLock Lock(&LoadResolveMapLock);
		const auto CachedResolveMap = ResolveMapCache.Find(LazyRulePackagePtr);
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
		ScheduledTaskEvent = ResolveMapEventGraphRefCache.Find(LazyRulePackagePtr);
	}
	if (ScheduledTaskEvent)
	{
		// Add task which only fetches the result from the cache once the actual loading has finished
		FGraphEventArray Prerequisites;
		Prerequisites.Add(*ScheduledTaskEvent);
		TGraphTask<TAsyncGraphTask<ResolveMapSPtr>>::CreateTask(&Prerequisites)
			.ConstructAndDispatchWhenReady(
				[this, LazyRulePackagePtr]() {
					FScopeLock Lock(&LoadResolveMapLock);
					return ResolveMapCache[LazyRulePackagePtr];
				},
				MoveTemp(Promise), ENamedThreads::AnyThread);
	}
	else
	{
		FGraphEventRef LoadTask;
		{
			const FString PathName = RulePackage->GetPathName();
			const std::wstring PathUri = UnrealResolveMapProvider::SCHEME_UNREAL + L":" + TCHAR_TO_WCHAR(*PathName);
			const FString Uri = WCHAR_TO_TCHAR(PathUri.c_str());

			FScopeLock Lock(&LoadResolveMapLock);
			// Task which does the actual resolve map loading which might take a long time
			LoadTask = TGraphTask<FLoadResolveMapTask>::CreateTask().ConstructAndDispatchWhenReady(MoveTemp(Promise), LazyRulePackagePtr, Uri,
																								   ResolveMapCache, LoadResolveMapLock);
			ResolveMapEventGraphRefCache.Add(LazyRulePackagePtr, LoadTask);
		}

		// Task which removes the event from the cache once finished
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[this, LazyRulePackagePtr]() {
				FScopeLock Lock(&LoadResolveMapLock);
				ResolveMapEventGraphRefCache.Remove(LazyRulePackagePtr);
			},
			TStatId(), LoadTask, ENamedThreads::AnyThread);
	}

	return Future;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(VitruvioModule, Vitruvio)
