// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/StaticMesh.h"
#include "Modules/ModuleManager.h"
#include "RuleAttributes.h"
#include "RulePackage.h"
#include "UnrealLogHandler.h"
#include "PRTTypes.h"

#include "prt/Object.h"

#include <map>
#include <memory>
#include <string>


DECLARE_LOG_CATEGORY_EXTERN(LogUnrealPrt, Log, All);

struct FGenerateResult
{
	UStaticMesh* ShapeMesh;
	TMap<UStaticMesh*, TArray<FTransform>> Instances;
};

class UnrealGeometryEncoderModule : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;

	
	/**
	 * \brief Calls prt generate with the given InitialShape, RulePackage and Attributes. The instances are written into OutInstances.
	 * 
	 * \param InitialShape
	 * \param OpaqueParent 
	 * \param RulePackage 
	 * \param Attributes 
	 * \return the generated UStaticMesh.
	 */
	UNREALGEOMETRYENCODER_API FGenerateResult Generate(const UStaticMesh* InitialShape, UMaterial* OpaqueParent, URulePackage* RulePackage, const TMap<FString, URuleAttribute*>& Attributes) const;

	UNREALGEOMETRYENCODER_API void LoadDefaultRuleAttributes(const UStaticMesh* InitialShape, URulePackage* RulePackage, TMap<FString, URuleAttribute*>& OutAttributes) const;

	static UnrealGeometryEncoderModule& Get()
	{
		return FModuleManager::LoadModuleChecked<UnrealGeometryEncoderModule>("UnrealGeometryEncoder");
	}

private:
	void* PrtDllHandle = nullptr;
	prt::Object const* PrtLibrary = nullptr;
	bool Initialized = false;
	CacheObjectUPtr PrtCache;

	UnrealLogHandler* LogHandler = nullptr;

	mutable std::map<std::wstring, ResolveMapSPtr> ResolveMapCache;

	ResolveMapSPtr GetResolveMap(const std::wstring& Uri) const;
};
