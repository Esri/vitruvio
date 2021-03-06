/* Copyright 2021 Esri
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
#include "Interfaces/IPluginManager.h"
#include "MeshDescription.h"
#include "Modules/ModuleManager.h"
#include "StaticMeshAttributes.h"
#include "UObject/GCObjectScopeGuard.h"
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

		const FString RpkFile = FPaths::GetBaseFilename(UriPath, true) + TEXT(".rpk");
		const FString RpkPath = FPaths::Combine(RpkFolder, RpkFile);
		PlatformFile.CreateDirectoryTree(*RpkFolder);
		IFileHandle* RpkHandle = PlatformFile.OpenWrite(*RpkPath);
		if (RpkHandle)
		{
			// Write file to disk
			RpkHandle->Write(LazyRulePackagePtr->Data.GetData(), LazyRulePackagePtr->Data.Num());
			RpkHandle->Flush();
			delete RpkHandle;

			// Create rpk
			const std::wstring AbsoluteRpkPath(TCHAR_TO_WCHAR(*FPaths::ConvertRelativePathToFull(RpkPath)));
			const std::wstring AbsoluteRpkFolder(TCHAR_TO_WCHAR(*FPaths::Combine(FPaths::GetPath(FPaths::ConvertRelativePathToFull(RpkPath)),
																				 FPaths::GetBaseFilename(UriPath, true) + TEXT("_Unpacked"))));
			const std::wstring RpkFileUri = prtu::toFileURI(AbsoluteRpkPath);

			prt::Status Status;
			const ResolveMapSPtr ResolveMapPtr(prt::createResolveMap(RpkFileUri.c_str(), AbsoluteRpkFolder.c_str(), &Status), PRTDestroyer());
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

void SetInitialShapeGeometry(const InitialShapeBuilderUPtr& InitialShapeBuilder, const TArray<FInitialShapeFace>& InitialShape)
{
	std::vector<double> vertexCoords;
	std::vector<uint32_t> indices;
	std::vector<uint32_t> faceCounts;

	uint32_t CurrentIndex = 0;
	for (const FInitialShapeFace& Face : InitialShape)
	{
		faceCounts.push_back(Face.Vertices.Num());
		for (const FVector& Vertex : Face.Vertices)
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
										   const TArray<FInitialShapeFace>& InitialShape, prt::Cache* Cache, const int32 RandomSeed)
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

	LogHandler = new UnrealLogHandler;
	prt::addLogHandler(LogHandler);

	prt::Status Status;
	PrtLibrary = prt::init(PRTPluginsPaths.GetData(), PRTPluginsPaths.Num(), prt::LogLevel::LOG_TRACE, &Status);
	Initialized = Status == prt::STATUS_OK;

	PrtCache.reset(prt::CacheObject::create(prt::CacheObject::CACHE_TYPE_NONREDUNDANT));

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

	delete LogHandler;
}

FGenerateResult VitruvioModule::GenerateAsync(const TArray<FInitialShapeFace>& InitialShape, UMaterial* OpaqueParent, UMaterial* MaskedParent,
											  UMaterial* TranslucentParent, URulePackage* RulePackage, AttributeMapUPtr Attributes,
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
		FGenerateResultDescription Result =
			Generate(InitialShape, OpaqueParent, MaskedParent, TranslucentParent, RulePackage, std::move(AttributeMap), RandomSeed);
		return FGenerateResult::ResultType{Token, MoveTemp(Result)};
	});

	return FGenerateResult{MoveTemp(ResultFuture), Token};
}

FGenerateResultDescription VitruvioModule::Generate(const TArray<FInitialShapeFace>& InitialShape, UMaterial* OpaqueParent, UMaterial* MaskedParent,
													UMaterial* TranslucentParent, URulePackage* RulePackage, AttributeMapUPtr Attributes,
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
	const TSharedPtr<UnrealCallbacks> OutputHandler(new UnrealCallbacks(AttributeMapBuilder, OpaqueParent, MaskedParent, TranslucentParent));

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

	GenerateCallsCounter.Decrement();

	return {OutputHandler->GetInstances(), OutputHandler->GetMeshes(), OutputHandler->GetMaterials()};
}

FAttributeMapResult VitruvioModule::LoadDefaultRuleAttributesAsync(const TArray<FInitialShapeFace>& InitialShape, URulePackage* RulePackage,
																   const int32 RandomSeed) const
{
	check(RulePackage);

	FAttributeMapResult::FTokenPtr InvalidationToken = MakeShared<FInvalidationToken>();

	if (!Initialized)
	{
		UE_LOG(LogUnrealPrt, Warning, TEXT("PRT not initialized"))
		TPromise<FAttributeMapResult::ResultType> Result;
		Result.SetValue({InvalidationToken, nullptr});
		return {Result.GetFuture(), InvalidationToken};
	}

	LoadAttributesCounter.Increment();

	FAttributeMapResult::FFutureType AttributeMapPtrFuture = Async(EAsyncExecution::Thread, [=]() {
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
			GetDefaultAttributeValues(RuleFile.c_str(), StartRule.c_str(), ResolveMap, InitialShape, PrtCache.get(), RandomSeed));

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
