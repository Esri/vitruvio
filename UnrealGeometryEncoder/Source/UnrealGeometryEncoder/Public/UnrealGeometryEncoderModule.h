// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/StaticMesh.h"
#include "Modules/ModuleManager.h"
#include "RuleAttributes.h"
#include "RulePackage.h"
#include "UnrealLogHandler.h"

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

	UNREALGEOMETRYENCODER_API TArray<UStaticMesh*> Generate(const UStaticMesh* InitialShape, URulePackage* RulePackage, const TMap<FString, URuleAttribute*>& Attributes) const;

	UNREALGEOMETRYENCODER_API void SetDefaultRuleAttributes(const UStaticMesh* InitialShape, URulePackage* RulePackage, TMap<FString, URuleAttribute*>& OutAttributes) const;

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
