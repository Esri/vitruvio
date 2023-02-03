/* Copyright 2023 Esri
 *
 * Licensed under the Apache License Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "VitruvioModule.h"

#include "AsyncHelpers.h"
#include "PRTTypes.h"
#include "PRTUtils.h"
#include "UnrealCallbacks.h"

#include "Util/AttributeConversion.h"
#include "Util/MaterialConversion.h"
#include "Util/PolygonWindings.h"

#include "prt/API.h"
#include "prtx/EncoderInfoBuilder.h"

#include "Core.h"
#include "IImageWrapper.h"
#include "Interfaces/IPluginManager.h"
#include "MeshDescription.h"
#include "Modules/ModuleManager.h"
#include "StaticMeshAttributes.h"
#include "TextureDecoding.h"
#include "UObject/UObjectBaseUtility.h"

#define LOCTEXT_NAMESPACE "VitruvioModule"

DEFINE_LOG_CATEGORY(LogUnrealPrt);

namespace
{
constexpr const wchar_t* ATTRIBUTE_EVAL_ENCODER_ID = L"com.esri.prt.core.AttributeEvalEncoder";

class FLoadResolveMapTask
{
	TLazyObjectPtr<URulePackage> LazyRulePackagePtr;
	TPromise<ResolveMapSPtr> Promise;
	TMap<TLazyObjectPtr<URulePackage>, ResolveMapSPtr>& ResolveMapCache;
	FCriticalSection& LoadResolveMapLock;
	FString RpkFolder;

public:
	FLoadResolveMapTask(TPromise<ResolveMapSPtr>&& InPromise, const FString RpkFolder, const TLazyObjectPtr<URulePackage> LazyRulePackagePtr,
						TMap<TLazyObjectPtr<URulePackage>, ResolveMapSPtr>& ResolveMapCache, FCriticalSection& LoadResolveMapLock)
		: LazyRulePackagePtr(LazyRulePackagePtr), Promise(MoveTemp(InPromise)), ResolveMapCache(ResolveMapCache),
		  LoadResolveMapLock(LoadResolveMapLock), RpkFolder(RpkFolder)
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
		const FString UriPath = LazyRulePackagePtr->GetPathName();

		// Create rpk on disk for PRT
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		const FString RpkFile = FPaths::GetBaseFilename(UriPath, false) + TEXT(".rpk");
		const FString RpkFilePath = FPaths::Combine(RpkFolder, RpkFile);
		const FString RpkFolderPath = FPaths::GetPath(RpkFilePath);

		IFileManager::Get().Delete(*RpkFilePath);
		PlatformFile.CreateDirectoryTree(*RpkFolderPath);

		IFileHandle* RpkHandle = PlatformFile.OpenWrite(*RpkFilePath);
		if (RpkHandle)
		{
			// Write file to disk
			RpkHandle->Write(LazyRulePackagePtr->Data.GetData(), LazyRulePackagePtr->Data.Num());
			RpkHandle->Flush();
			delete RpkHandle;

			// Create rpk
			const std::wstring AbsoluteRpkPath(TCHAR_TO_WCHAR(*FPaths::ConvertRelativePathToFull(RpkFilePath)));

			const std::wstring RpkFileUri = prtu::toFileURI(AbsoluteRpkPath);
			prt::Status Status;
			const ResolveMapSPtr ResolveMapPtr(prt::createResolveMap(RpkFileUri.c_str(), nullptr, &Status), PRTDestroyer());
			{
				FScopeLock Lock(&LoadResolveMapLock);
				ResolveMapCache.Add(LazyRulePackagePtr, ResolveMapPtr);
				Promise.SetValue(ResolveMapPtr);
			}
		}
		else
		{
			Promise.SetValue(nullptr);
		}
	}
};

void SetInitialShapeGeometry(const InitialShapeBuilderUPtr& InitialShapeBuilder, const FInitialShapePolygon& InitialShape)
{
	std::vector<double> vertexCoords;
	std::vector<uint32_t> indices;
	std::vector<uint32_t> faceCounts;
	std::vector<uint32_t> holes;

	for (const FVector3f& Vertex : InitialShape.Vertices)
	{
		const FVector CEVertex = FVector(Vertex.X, Vertex.Z, Vertex.Y) / 100.0;
		vertexCoords.push_back(CEVertex.X);
		vertexCoords.push_back(CEVertex.Y);
		vertexCoords.push_back(CEVertex.Z);
	}

	for (const FInitialShapeFace& Face : InitialShape.Faces)
	{
		faceCounts.push_back(Face.Indices.Num());
		for (const int32& Index : Face.Indices)
		{
			indices.push_back(Index);
		}

		if (Face.Holes.Num() > 0)
		{
			holes.push_back(faceCounts.size() - 1);

			for (const FInitialShapeHole Hole : Face.Holes)
			{
				faceCounts.push_back(Hole.Indices.Num());
				for (const int32& Index : Hole.Indices)
				{
					indices.push_back(Index);
				}
				holes.push_back(faceCounts.size() - 1);
			}

			holes.push_back(std::numeric_limits<uint32_t>::max());
		}
	}

	const prt::Status SetGeometryStatus = InitialShapeBuilder->setGeometry(vertexCoords.data(), vertexCoords.size(), indices.data(), indices.size(),
																		   faceCounts.data(), faceCounts.size(), holes.data(), holes.size());

	if (SetGeometryStatus != prt::STATUS_OK)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("InitialShapeBuilder setGeometry failed status = %hs"), prt::getStatusDescription(SetGeometryStatus))
	}

	for (int32 UVSet = 0; UVSet < 8; ++UVSet)
	{
		std::vector<double> uvCoords;
		std::vector<uint32_t> uvIndices;

		uint32_t CurrentUVIndex = 0;
		if (UVSet >= InitialShape.TextureCoordinateSets.Num())
		{
			continue;
		}

		for (const auto& UV : InitialShape.TextureCoordinateSets[UVSet].TextureCoordinates)
		{
			uvIndices.push_back(CurrentUVIndex++);
			uvCoords.push_back(UV.X);
			uvCoords.push_back(-UV.Y);
		}

		if (uvCoords.empty())
		{
			continue;
		}

		InitialShapeBuilder->setUVs(uvCoords.data(), uvCoords.size(), uvIndices.data(), uvIndices.size(), faceCounts.data(), faceCounts.size(),
									UVSet);
	}
}

AttributeMapUPtr EvaluateRuleAttributes(const std::wstring& RuleFile, const std::wstring& StartRule, AttributeMapUPtr Attributes,
										const ResolveMapSPtr& ResolveMapPtr, const FInitialShapePolygon& InitialShape, prt::Cache* Cache,
										const int32 RandomSeed)
{
	AttributeMapBuilderUPtr UnrealCallbacksAttributeBuilder(prt::AttributeMapBuilder::create());
	UnrealCallbacks UnrealCallbacks(UnrealCallbacksAttributeBuilder);

	InitialShapeBuilderUPtr InitialShapeBuilder(prt::InitialShapeBuilder::create());

	SetInitialShapeGeometry(InitialShapeBuilder, InitialShape);

	InitialShapeBuilder->setAttributes(RuleFile.c_str(), StartRule.c_str(), RandomSeed, L"", Attributes.get(), ResolveMapPtr.get());

	const InitialShapeUPtr Shape(InitialShapeBuilder->createInitialShapeAndReset());
	const InitialShapeNOPtrVector InitialShapes = {Shape.get()};

	const std::vector<const wchar_t*> EncoderIds = {ATTRIBUTE_EVAL_ENCODER_ID};
	const AttributeMapUPtr AttributeEncodeOptions = prtu::createValidatedOptions(ATTRIBUTE_EVAL_ENCODER_ID);
	const AttributeMapNOPtrVector EncoderOptions = {AttributeEncodeOptions.get()};

	prt::generate(InitialShapes.data(), InitialShapes.size(), nullptr, EncoderIds.data(), EncoderIds.size(), EncoderOptions.data(), &UnrealCallbacks,
				  Cache, nullptr);

	return AttributeMapUPtr(UnrealCallbacksAttributeBuilder->createAttributeMap());
}

void CleanupTempRpkFolder()
{
	FString TempDir(WCHAR_TO_TCHAR(prtu::temp_directory_path().c_str()));
	const FString RpkUnpackFolder = FPaths::Combine(TempDir, TEXT("PRT"), TEXT("UnrealGeometryEncoder"));
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.DeleteDirectoryRecursively(*RpkUnpackFolder);
}

FString GetPlatformName()
{
#if PLATFORM_64BITS && PLATFORM_WINDOWS
	return "Win64";
#elif PLATFORM_MAC
	return "Mac";
#else
	return "Unknown";
#endif
}

FString GetPrtThirdPartyPath()
{
	const FString BaseDir = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin("Vitruvio")->GetBaseDir());
	const FString BinariesPath = FPaths::Combine(*BaseDir, TEXT("Source"), TEXT("ThirdParty"), TEXT("PRT"));
	return BinariesPath;
}

FString GetEncoderExtensionPath()
{
	const FString BaseDir = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin("Vitruvio")->GetBaseDir());
	const FString BinariesPath = FPaths::Combine(*BaseDir, TEXT("Source"), TEXT("ThirdParty"), TEXT("UnrealGeometryEncoderLib"), TEXT("lib"),
												 GetPlatformName(), TEXT("Release"));
	return BinariesPath;
}

FString GetPrtLibDir()
{
	const FString BaseDir = GetPrtThirdPartyPath();
	const FString LibDir = FPaths::Combine(*BaseDir, TEXT("lib"), GetPlatformName(), TEXT("Release"));
	return LibDir;
}

FString GetPrtBinDir()
{
	const FString BaseDir = GetPrtThirdPartyPath();
	const FString BinDir = FPaths::Combine(*BaseDir, TEXT("bin"), GetPlatformName(), TEXT("Release"));
	return BinDir;
}

FString GetPrtDllPath()
{
	const FString BaseDir = GetPrtBinDir();
	return FPaths::Combine(*BaseDir, TEXT("com.esri.prt.core.dll"));
}

} // namespace

void VitruvioModule::InitializePrt()
{
	const FString PrtLibPath = GetPrtDllPath();
	const FString PrtBinDir = GetPrtBinDir();
	const FString PrtLibDir = GetPrtLibDir();

	FPlatformProcess::AddDllDirectory(*PrtBinDir);
	FPlatformProcess::AddDllDirectory(*PrtLibDir);
	PrtDllHandle = FPlatformProcess::GetDllHandle(*PrtLibPath);

	TArray<wchar_t*> PRTPluginsPaths;
	const FString EncoderExtensionPath = GetEncoderExtensionPath();
	const FString PrtExtensionPaths = GetPrtLibDir();
	PRTPluginsPaths.Add(const_cast<wchar_t*>(TCHAR_TO_WCHAR(*EncoderExtensionPath)));
	PRTPluginsPaths.Add(const_cast<wchar_t*>(TCHAR_TO_WCHAR(*PrtExtensionPaths)));

	LogHandler = MakeUnique<UnrealLogHandler>();
	prt::addLogHandler(LogHandler.Get());

	prt::Status Status;
	PrtLibrary = prt::init(PRTPluginsPaths.GetData(), PRTPluginsPaths.Num(), prt::LogLevel::LOG_TRACE, &Status);
	Initialized = Status == prt::STATUS_OK;

	PrtCache.reset(prt::CacheObject::create(prt::CacheObject::CACHE_TYPE_DEFAULT));

	const FString TempDir(WCHAR_TO_TCHAR(prtu::temp_directory_path().c_str()));
	RpkFolder = FPaths::CreateTempFilename(*TempDir, TEXT("Vitruvio_"), TEXT(""));
}

void VitruvioModule::StartupModule()
{
	// During cooking we do not start Vitruvio
	if (IsRunningCommandlet())
	{
		return;
	}

	InitializePrt();
}

void VitruvioModule::ShutdownModule()
{
	if (!Initialized)
	{
		return;
	}

	Initialized = false;

	UE_LOG(LogUnrealPrt, Display,
		   TEXT("Shutting down Vitruvio. Waiting for ongoing generate calls (%d), RPK loading tasks (%d) and attribute loading tasks (%d)"),
		   GenerateCallsCounter.GetValue(), RpkLoadingTasksCounter.GetValue(), LoadAttributesCounter.GetValue())

	// Wait until no more PRT calls are ongoing
	FGenericPlatformProcess::ConditionalSleep(
		[this]() { return GenerateCallsCounter.GetValue() == 0 && RpkLoadingTasksCounter.GetValue() == 0 && LoadAttributesCounter.GetValue() == 0; },
		0); // Yield to other threads

	UE_LOG(LogUnrealPrt, Display, TEXT("PRT calls finished. Shutting down."))

	if (PrtDllHandle)
	{
		FPlatformProcess::FreeDllHandle(PrtDllHandle);
		PrtDllHandle = nullptr;
	}
	if (PrtLibrary)
	{
		PrtLibrary->destroy();
	}

	CleanupTempRpkFolder();

	UE_LOG(LogUnrealPrt, Display, TEXT("Shutdown complete"))
}

Vitruvio::FTextureData VitruvioModule::DecodeTexture(UObject* Outer, const FString& Path, const FString& Key) const
{
	const prt::AttributeMap* TextureMetadataAttributeMap = prt::createTextureMetadata(*Path, PrtCache.get());
	Vitruvio::FTextureMetadata TextureMetadata = Vitruvio::ParseTextureMetadata(TextureMetadataAttributeMap);

	size_t BufferSize = TextureMetadata.Width * TextureMetadata.Height * TextureMetadata.Bands * TextureMetadata.BytesPerBand;
	auto Buffer = std::make_unique<uint8_t[]>(BufferSize);

	prt::getTexturePixeldata(*Path, Buffer.get(), BufferSize, PrtCache.get());

	return Vitruvio::DecodeTexture(Outer, Key, Path, TextureMetadata, std::move(Buffer), BufferSize);
}

FGenerateResult VitruvioModule::GenerateAsync(const FInitialShapePolygon& InitialShape, URulePackage* RulePackage, AttributeMapUPtr Attributes,
											  const int32 RandomSeed) const
{
	check(RulePackage);

	const FGenerateResult::FTokenPtr Token = MakeShared<FGenerateToken>();

	if (!Initialized)
	{
		UE_LOG(LogUnrealPrt, Warning, TEXT("PRT not initialized"))

		TPromise<FGenerateResult::ResultType> Result;
		Result.SetValue({Token, {}});
		return {
			Result.GetFuture(),
			Token,
		};
	}

	FGenerateResult::FFutureType ResultFuture = Async(EAsyncExecution::Thread, [=, AttributeMap = std::move(Attributes)]() mutable {
		FGenerateResultDescription Result = Generate(InitialShape, RulePackage, std::move(AttributeMap), RandomSeed);
		return FGenerateResult::ResultType{Token, MoveTemp(Result)};
	});

	return FGenerateResult{MoveTemp(ResultFuture), Token};
}

FGenerateResultDescription VitruvioModule::Generate(const FInitialShapePolygon& InitialShape, URulePackage* RulePackage, AttributeMapUPtr Attributes,
													const int32 RandomSeed) const
{
	check(RulePackage);

	if (!Initialized)
	{
		UE_LOG(LogUnrealPrt, Warning, TEXT("PRT not initialized"))
		return {};
	}

	GenerateCallsCounter.Increment();

	const InitialShapeBuilderUPtr InitialShapeBuilder(prt::InitialShapeBuilder::create());
	SetInitialShapeGeometry(InitialShapeBuilder, InitialShape);

	const ResolveMapSPtr ResolveMap = LoadResolveMapAsync(RulePackage).Get();

	const std::wstring RuleFile = prtu::getRuleFileEntry(ResolveMap);
	const wchar_t* RuleFileUri = ResolveMap->getString(RuleFile.c_str());

	const RuleFileInfoUPtr StartRuleInfo(prt::createRuleFileInfo(RuleFileUri));
	const std::wstring StartRule = prtu::detectStartRule(StartRuleInfo);

	InitialShapeBuilder->setAttributes(RuleFile.c_str(), StartRule.c_str(), RandomSeed, L"", Attributes.get(), ResolveMap.get());

	AttributeMapBuilderUPtr AttributeMapBuilder(prt::AttributeMapBuilder::create());
	const TSharedPtr<UnrealCallbacks> OutputHandler(new UnrealCallbacks(AttributeMapBuilder));

	const InitialShapeUPtr Shape(InitialShapeBuilder->createInitialShapeAndReset());

	const std::vector<const wchar_t*> EncoderIds = {UNREAL_GEOMETRY_ENCODER_ID};
	const AttributeMapUPtr UnrealEncoderOptions(prtu::createValidatedOptions(UNREAL_GEOMETRY_ENCODER_ID));
	const AttributeMapNOPtrVector EncoderOptions = {UnrealEncoderOptions.get()};

	InitialShapeNOPtrVector Shapes = {Shape.get()};

	const prt::Status GenerateStatus = prt::generate(Shapes.data(), Shapes.size(), nullptr, EncoderIds.data(), EncoderIds.size(),
													 EncoderOptions.data(), OutputHandler.Get(), PrtCache.get(), nullptr);

	if (GenerateStatus != prt::STATUS_OK)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("PRT generate failed: %hs"), prt::getStatusDescription(GenerateStatus))
	}

	const int GenerateCalls = GenerateCallsCounter.Decrement();

	if (!Initialized)
	{
		return {};
	}

	// Notify generate complete callback on game thread
	AsyncTask(ENamedThreads::GameThread, [this, GenerateCalls]() {
		if (!Initialized)
		{
			return;
		}

		OnGenerateCompleted.Broadcast(GenerateCalls);

		if (GenerateCalls == 0)
		{
			TArray<FLogMessage> Messages = LogHandler->PopMessages();

			int Warnings = 0;
			int Errors = 0;

			for (const FLogMessage& Message : Messages)
			{
				if (Message.Level == prt::LOG_WARNING)
				{
					Warnings++;
				}
				else if (Message.Level == prt::LOG_ERROR || Message.Level == prt::LOG_FATAL)
				{
					Errors++;
				}
			}

			OnAllGenerateCompleted.Broadcast(Warnings, Errors);
		}
	});

	return FGenerateResultDescription{OutputHandler->GetInstances(), OutputHandler->GetMeshes(), OutputHandler->GetReports(),
									  OutputHandler->GetNames()};
}

FAttributeMapResult VitruvioModule::EvaluateRuleAttributesAsync(const FInitialShapePolygon& InitialShape, URulePackage* RulePackage,
																AttributeMapUPtr Attributes, const int32 RandomSeed) const
{
	check(RulePackage);

	FAttributeMapResult::FTokenPtr InvalidationToken = MakeShared<FEvalAttributesToken>();

	if (!Initialized)
	{
		UE_LOG(LogUnrealPrt, Warning, TEXT("PRT not initialized"))
		TPromise<FAttributeMapResult::ResultType> Result;
		Result.SetValue({InvalidationToken, nullptr});
		return {Result.GetFuture(), InvalidationToken};
	}

	LoadAttributesCounter.Increment();

	FAttributeMapResult::FFutureType AttributeMapPtrFuture = Async(EAsyncExecution::Thread, [=, Attributes = std::move(Attributes)]() mutable {
		const ResolveMapSPtr ResolveMap = LoadResolveMapAsync(RulePackage).Get();

		const std::wstring RuleFile = prtu::getRuleFileEntry(ResolveMap);
		const wchar_t* RuleFileUri = ResolveMap->getString(RuleFile.c_str());

		const RuleFileInfoUPtr StartRuleInfo(prt::createRuleFileInfo(RuleFileUri));
		const std::wstring StartRule = prtu::detectStartRule(StartRuleInfo);

		prt::Status InfoStatus;
		RuleFileInfoUPtr RuleInfo(prt::createRuleFileInfo(RuleFileUri, PrtCache.get(), &InfoStatus));
		if (!RuleInfo || InfoStatus != prt::STATUS_OK)
		{
			UE_LOG(LogUnrealPrt, Error, TEXT("could not get rule file info from rule file %s"), RuleFileUri)
			return FAttributeMapResult::ResultType{
				InvalidationToken,
				nullptr,
			};
		}

		AttributeMapUPtr DefaultAttributeMap(
			EvaluateRuleAttributes(RuleFile.c_str(), StartRule.c_str(), std::move(Attributes), ResolveMap, InitialShape, PrtCache.get(), RandomSeed));

		LoadAttributesCounter.Decrement();

		if (!Initialized)
		{
			return FAttributeMapResult::ResultType{InvalidationToken, nullptr};
		}

		const TSharedPtr<FAttributeMap> AttributeMap = MakeShared<FAttributeMap>(std::move(DefaultAttributeMap), std::move(RuleInfo));
		return FAttributeMapResult::ResultType{InvalidationToken, AttributeMap};
	});

	return {MoveTemp(AttributeMapPtrFuture), InvalidationToken};
}

void VitruvioModule::EvictFromResolveMapCache(URulePackage* RulePackage)
{
	const TLazyObjectPtr<URulePackage> LazyRulePackagePtr(RulePackage);
	FScopeLock Lock(&LoadResolveMapLock);
	ResolveMapCache.Remove(LazyRulePackagePtr);
	PrtCache->flushAll();
}

void VitruvioModule::RegisterMesh(UStaticMesh* StaticMesh)
{
	FScopeLock Lock(&RegisterMeshLock);
	RegisteredMeshes.Add(StaticMesh);
}

void VitruvioModule::UnregisterMesh(UStaticMesh* StaticMesh)
{
	FScopeLock Lock(&RegisterMeshLock);
	RegisteredMeshes.Remove(StaticMesh);
}

TFuture<ResolveMapSPtr> VitruvioModule::LoadResolveMapAsync(URulePackage* const RulePackage) const
{
	TPromise<ResolveMapSPtr> Promise;
	TFuture<ResolveMapSPtr> Future = Promise.GetFuture();

	if (!Initialized)
	{
		Promise.SetValue({});
		return Future;
	}

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
		RpkLoadingTasksCounter.Increment();

		FGraphEventRef LoadTask;
		{
			FScopeLock Lock(&LoadResolveMapLock);
			// Task which does the actual resolve map loading which might take a long time
			LoadTask = TGraphTask<FLoadResolveMapTask>::CreateTask().ConstructAndDispatchWhenReady(MoveTemp(Promise), RpkFolder, LazyRulePackagePtr,
																								   ResolveMapCache, LoadResolveMapLock);
			ResolveMapEventGraphRefCache.Add(LazyRulePackagePtr, LoadTask);
		}

		// Task which removes the event from the cache once finished
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[this, LazyRulePackagePtr]() {
				FScopeLock Lock(&LoadResolveMapLock);
				RpkLoadingTasksCounter.Decrement();
				ResolveMapEventGraphRefCache.Remove(LazyRulePackagePtr);
			},
			TStatId(), LoadTask, ENamedThreads::AnyThread);
	}

	return Future;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(VitruvioModule, Vitruvio)
