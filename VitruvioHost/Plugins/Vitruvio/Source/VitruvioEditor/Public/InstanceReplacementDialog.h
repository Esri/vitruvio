// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InstanceReplacement.h"

#include "InstanceReplacementDialog.generated.h"

UCLASS(DisplayName = "Instance Replacement")
class UInstanceReplacementWrapper : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString SourceMeshIdentifier;

	UPROPERTY()
	TArray<UStaticMeshComponent*> MeshComponents;

	UPROPERTY(EditAnywhere, DisplayName = "Options", Category = "Replacements")
	TArray<FReplacementOption> Replacements;
};

UCLASS(meta = (DisplayName = "Options"))
class UInstanceReplacementDialogOptions : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	UInstanceReplacementAsset* TargetReplacementAsset;

	UPROPERTY(EditAnywhere)
	TMap<FString, UInstanceReplacementWrapper*> InstanceReplacements;
};

class FInstanceReplacementDialog
{
public:
	static void OpenDialog(class UVitruvioComponent* VitruvioComponent);
};
