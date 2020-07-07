// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "AttributeMap.h"
#include "PRTTypes.h"
#include "RuleAttributes.h"
#include "RulePackage.h"

#include "prt/Object.h"

#include "Engine/StaticMesh.h"
#include "Modules/ModuleManager.h"
#include "UnrealLogHandler.h"

#include <map>
#include <memory>
#include <string>

DECLARE_LOG_CATEGORY_EXTERN(LogUnrealPrt, Log, All);

struct FGenerateResult
{
	UStaticMesh* ShapeMesh;
	TMap<UStaticMesh*, TArray<FTransform>> Instances;
};

enum class EPrtState
{
	Uninitialized,
	Downloading,
	Unzipping,
	Installing,
	Initialized
};

class VitruvioModule final : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;

	/**
	 * \brief Asynchronously generate the models with the given InitialShape, RulePackage and Attributes.
	 *
	 * \param InitialShape
	 * \param OpaqueParent
	 * \param MaskedParent
	 * \param TranslucentParent
	 * \param RulePackage
	 * \param Attributes
	 * \param RandomSeed
	 * \return the generated UStaticMesh.
	 */
	VITRUVIO_API TFuture<FGenerateResult> GenerateAsync(const UStaticMesh* InitialShape, UMaterial* OpaqueParent, UMaterial* MaskedParent,
														UMaterial* TranslucentParent, URulePackage* RulePackage,
														const TMap<FString, URuleAttribute*>& Attributes, const int32 RandomSeed) const;

	/**
	 * \brief Generate the models with the given InitialShape, RulePackage and Attributes.
	 *
	 * \param InitialShape
	 * \param OpaqueParent
	 * \param MaskedParent
	 * \param TranslucentParent
	 * \param RulePackage
	 * \param Attributes
	 * \param RandomSeed
	 * \return the generated UStaticMesh.
	 */
	VITRUVIO_API FGenerateResult Generate(const UStaticMesh* InitialShape, ::UMaterial* OpaqueParent, UMaterial* MaskedParent,
										  UMaterial* TranslucentParent, URulePackage* RulePackage, const TMap<FString, URuleAttribute*>& Attributes,
										  const int32 RandomSeed) const;

	/**
	 * \brief Asynchronously loads the default attribute values for the given initial shape and rule package
	 *
	 * \param InitialShape
	 * \param RulePackage
	 * \param RandomSeed
	 * \return
	 */
	VITRUVIO_API TFuture<FAttributeMap> LoadDefaultRuleAttributesAsync(const UStaticMesh* InitialShape, URulePackage* RulePackage,
																	   const int32 RandomSeed) const;

	/**
	 * \brief Returns whether PRT is initialized meaning installed and ready to use. Before initialization generation is not possible and will
	 * immediately return without results.
	 */
	VITRUVIO_API bool IsInitialized() const { return State == EPrtState::Initialized; }

	/**
	 * \return the download progress if PRT is currently being downloaded. If no download is in progress it will return an empty TOptional.
	 */
	VITRUVIO_API TOptional<double> GetDownloadProgress() const { return DownloadProgress; }

	/**
	 * \return the install progress if PRT is currently being installed. If no installation is in progress it will return an empty TOptional.
	 */
	VITRUVIO_API TOptional<double> GetInstallProgress() const { return InstallProgress; }

	static VitruvioModule& Get() { return FModuleManager::LoadModuleChecked<VitruvioModule>("Vitruvio"); }

private:
	void* PrtDllHandle = nullptr;
	prt::Object const* PrtLibrary = nullptr;
	CacheObjectUPtr PrtCache;

	UnrealLogHandler* LogHandler = nullptr;

	EPrtState State = EPrtState::Uninitialized;

	FSimpleDelegate UnzipProgressDelegate;
	TOptional<double> DownloadProgress;
	TOptional<double> InstallProgress;
	int32 DownloadSizeBytes = 0;

	mutable TMap<TLazyObjectPtr<URulePackage>, ResolveMapSPtr> ResolveMapCache;
	mutable TMap<TLazyObjectPtr<URulePackage>, FGraphEventRef> ResolveMapEventGraphRefCache;

	mutable FCriticalSection LoadResolveMapLock;

	TFuture<ResolveMapSPtr> LoadResolveMapAsync(URulePackage* RulePackage) const;

	void DownloadPrt();
	void InstallPrt();

	void InitializePrt();
};
