// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "VitruvioEditorModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "RulePackageAssetTypeActions.h"

#include "Core.h"
#include "PRTActorDetails.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "VitruvioEditorModule"

void VitruvioEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FRulePackageAssetTypeActions()));

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout("PRTActor", FOnGetDetailCustomizationInstance::CreateStatic(&PRTActorDetails::MakeInstance));
}

void VitruvioEditorModule::ShutdownModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout("PRTActor");
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(VitruvioEditorModule, VitruvioEditor)
