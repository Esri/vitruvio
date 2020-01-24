// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "VitruvioEditorModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "RulePackageAssetTypeActions.h"

#include "Core.h"
#include "ModuleManager.h"

#define LOCTEXT_NAMESPACE "VitruvioEditorModule"

void VitruvioEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	AssetTools.RegisterAssetTypeActions(MakeShareable(new FRulePackageAssetTypeActions()));
}

void VitruvioEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(VitruvioEditorModule, VitruvioEditor)
