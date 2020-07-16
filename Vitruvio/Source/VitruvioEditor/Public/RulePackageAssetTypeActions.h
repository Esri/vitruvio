// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"
#include "UnrealEd.h"

class FRulePackageAssetTypeActions final : public FAssetTypeActions_Base
{
public:
	FText GetName() const override;
	FColor GetTypeColor() const override;
	UClass* GetSupportedClass() const override;
	void OpenAssetEditor(const TArray<UObject*>& InObjects,
						 TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	uint32 GetCategories() override;
};
