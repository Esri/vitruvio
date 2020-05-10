// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "VitruvioEditor/Public/RulePackageAssetTypeActions.h"

#include "RulePackage.h"
#include "UnrealEd.h"

FText FRulePackageAssetTypeActions::GetName() const
{
	return FText::FromString("Esri Rule Package");
}

FColor FRulePackageAssetTypeActions::GetTypeColor() const
{
	return FColor(255, 0, 0);
}

UClass* FRulePackageAssetTypeActions::GetSupportedClass() const
{
	return URulePackage::StaticClass();
}

void FRulePackageAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
}

uint32 FRulePackageAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}
