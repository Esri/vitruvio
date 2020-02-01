// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/StaticMesh.h"
#include "Modules/ModuleManager.h"
#include "RuleAttributes.h"
#include "RulePackage.h"
#include "UnrealLogHandler.h"
#include "HierarchicalInstancedStaticMeshComponent.generated.h"

#include "prt/Object.h"
#include "prt/ResolveMap.h"

#include <map>
#include <memory>
#include <string>

DECLARE_LOG_CATEGORY_EXTERN(LogUnrealPrt, Log, All);

using ResolveMapSPtr = std::shared_ptr<prt::ResolveMap const>;

class UnrealGeometryEncoderModule : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;

	
	/**
	 * \brief Calls prt generate with the given InitialShape, RulePackage and Attributes. The instances are written into OutInstances.
	 * 
	 * \param InitialShape 
	 * \param RulePackage 
	 * \param Attributes 
	 * \param OutInstances 
	 * \return the generated UStaticMesh.
	 */
	UNREALGEOMETRYENCODER_API UStaticMesh* Generate(const UStaticMesh* InitialShape, URulePackage* RulePackage, const TMap<FString, URuleAttribute*>& Attributes, TArray<UInstancedStaticMeshComponent*>& OutInstances) const;

	UNREALGEOMETRYENCODER_API void LoadDefaultRuleAttributes(const UStaticMesh* InitialShape, URulePackage* RulePackage, TMap<FString, URuleAttribute*>& OutAttributes) const;

	static UnrealGeometryEncoderModule& Get()
	{
		return FModuleManager::LoadModuleChecked<UnrealGeometryEncoderModule>("UnrealGeometryEncoder");
	}

private:
	void* PrtDllHandle = nullptr;
	prt::Object const* PrtLibrary = nullptr;
	bool Initialized = false;

	UnrealLogHandler* LogHandler = nullptr;

	mutable std::map<std::wstring, ResolveMapSPtr> ResolveMapCache;

	ResolveMapSPtr GetResolveMap(const std::wstring& Uri) const;
};
