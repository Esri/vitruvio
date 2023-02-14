// Copyright © 2017-2022 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MaterialReplacement.h"

#include "MaterialReplacementDialog.generated.h"


UCLASS(BlueprintType)
class UMaterialReplacement : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	UMaterialInterface* Source;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* Replacement;
};

UCLASS(meta = (DisplayName = "Options"))
class UMaterialReplacementDialogOptions : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	UMaterialReplacementAsset* TargetReplacementAsset;

	UPROPERTY(EditAnywhere)
	TMap<FName, UMaterialReplacement*> MaterialReplacements;
};

class FMaterialReplacementDialog
{
public:
	static void OpenDialog(UStaticMeshComponent* SourceMeshComponent);
};
