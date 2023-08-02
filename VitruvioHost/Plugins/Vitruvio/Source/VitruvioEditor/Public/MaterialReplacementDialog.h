// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MaterialReplacement.h"
#include "ReplacementDialog.h"

#include "MaterialReplacementDialog.generated.h"

UCLASS(BlueprintType)
class UMaterialReplacement : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<UStaticMeshComponent*> Components;

	UPROPERTY()
	FName SourceMaterialSlot;

	UPROPERTY()
	UMaterialInterface* ReplacementMaterial;
};

USTRUCT()
struct FMaterialKey
{
	GENERATED_BODY()

	UPROPERTY()
	UMaterialInterface* Material;

	UPROPERTY()
	FName SourceMaterialSlot;

	friend bool operator==(const FMaterialKey& Lhs, const FMaterialKey& RHS)
	{
		return Lhs.Material == RHS.Material && Lhs.SourceMaterialSlot == RHS.SourceMaterialSlot;
	}

	friend bool operator!=(const FMaterialKey& Lhs, const FMaterialKey& RHS)
	{
		return !(Lhs == RHS);
	}

	friend uint32 GetTypeHash(const FMaterialKey& Object)
	{
		return HashCombine(GetTypeHash(Object.SourceMaterialSlot), GetTypeHash(Object.Material));
	}
};

UCLASS(meta = (DisplayName = "Options"))
class UMaterialReplacementDialogOptions : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	UMaterialReplacementAsset* TargetReplacementAsset;

	UPROPERTY(EditAnywhere)
	TMap<FMaterialKey, UMaterialReplacement*> MaterialReplacements;
};

class SMaterialReplacementDialogWidget : public SReplacementDialogWidget
{
	UMaterialReplacementDialogOptions* ReplacementDialogOptions = nullptr;

	TArray<TSharedPtr<SCheckBox>> IsolateCheckboxes;
	TSharedPtr<SCheckBox> IncludeInstancesCheckBox;
	TSharedPtr<SCheckBox> ApplyToAllVitruvioActorsCheckBox;

public:
	SLATE_BEGIN_ARGS(SMaterialReplacementDialogWidget) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_ARGUMENT(UVitruvioComponent*, VitruvioComponent)
	SLATE_END_ARGS()

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	void Construct(const FArguments& InArgs);

protected:
	virtual FText CreateHeaderText() override;
	virtual TSharedPtr<ISinglePropertyView> CreateTargetReplacementWidget() override;
	virtual void UpdateApplyButtonEnablement() override;
	virtual void OnCreateNewAsset() override;
	virtual void AddDialogOptions(const TSharedPtr<SVerticalBox>& Content) override;
	virtual void OnWindowClosed() override;
	virtual void UpdateReplacementTable() override;
	virtual FReply OnReplacementConfirmed() override;
	virtual FReply OnReplacementCanceled() override;
};

class FMaterialReplacementDialog
{
public:
	template <typename TOnWindowClosed>
	static void OpenDialog(UVitruvioComponent* VitruvioComponent, TOnWindowClosed OnWindowClosed)
	{
		FReplacementDialog::OpenDialog<SMaterialReplacementDialogWidget>(VitruvioComponent, OnWindowClosed);
	}
};
