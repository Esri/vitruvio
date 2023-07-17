// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MaterialReplacement.h"

#include "InstanceReplacementDialog.generated.h"

UCLASS(BlueprintType)
class UInstanceReplacement : public UObject
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
struct FInstanceKey
{
	GENERATED_BODY()

	UPROPERTY()
	FName SourceMeshName;
	
	UPROPERTY()
	UStaticMesh* StaticMesh;

	friend bool operator==(const FInstanceKey& Lhs, const FInstanceKey& RHS)
	{
		return Lhs.StaticMesh == RHS.StaticMesh && Lhs.SourceMeshName == RHS.SourceMeshName;
	}

	friend bool operator!=(const FInstanceKey& Lhs, const FInstanceKey& RHS)
	{
		return !(Lhs == RHS);
	}

	friend uint32 GetTypeHash(const FInstanceKey& Object)
	{
		return HashCombine(GetTypeHash(Object.SourceMeshName), GetTypeHash(Object.StaticMesh));
	}
};

UCLASS(meta = (DisplayName = "Options"))
class UInstanceReplacementDialogOptions : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	UMaterialReplacementAsset* TargetReplacementAsset;

	UPROPERTY(EditAnywhere)
	TMap<FInstanceKey, UInstanceReplacement*> InstanceReplacements;
};

class FInstanceReplacementDialog
{
public:
	static void OpenDialog(class UVitruvioComponent* VitruvioComponent);
};
