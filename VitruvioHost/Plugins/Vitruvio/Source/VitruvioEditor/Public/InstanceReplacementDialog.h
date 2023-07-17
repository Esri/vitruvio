// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InstanceReplacement.h"

#include "InstanceReplacementDialog.generated.h"

UCLASS(BlueprintType)
class UInstanceReplacement : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString SourceMeshIdentifier;
	
	UPROPERTY()
	UStaticMesh* ReplacementMesh;
};

USTRUCT()
struct FInstanceKey
{
	GENERATED_BODY()

	UPROPERTY()
	FString SourceMeshIdentifier;
	
	UPROPERTY()
	UStaticMeshComponent* MeshComponent;

	friend bool operator==(const FInstanceKey& Lhs, const FInstanceKey& RHS)
	{
		return Lhs.MeshComponent == RHS.MeshComponent && Lhs.SourceMeshIdentifier == RHS.SourceMeshIdentifier;
	}

	friend bool operator!=(const FInstanceKey& Lhs, const FInstanceKey& RHS)
	{
		return !(Lhs == RHS);
	}

	friend uint32 GetTypeHash(const FInstanceKey& Object)
	{
		return HashCombine(GetTypeHash(Object.SourceMeshIdentifier), GetTypeHash(Object.MeshComponent));
	}
};

UCLASS(meta = (DisplayName = "Options"))
class UInstanceReplacementDialogOptions : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	UInstanceReplacementAsset* TargetReplacementAsset;

	UPROPERTY(EditAnywhere)
	TMap<FInstanceKey, UInstanceReplacement*> InstanceReplacements;
};

class FInstanceReplacementDialog
{
public:
	static void OpenDialog(class UVitruvioComponent* VitruvioComponent);
};
