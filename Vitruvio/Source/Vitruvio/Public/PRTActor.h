// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "RuleAttributes.h"
#include "RulePackage.h"
#include "VitruvioModule.h"

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/Material.h"
#include "PRTActor.generated.h"

UCLASS()
class VITRUVIO_API APRTActor : public AStaticMeshActor
{
	GENERATED_BODY()

	TAtomic<bool> Initialized = false;
	TAtomic<bool> AttributesReady = false;

	TFuture<FGenerateResult> GenerateFuture;

public:
	APRTActor();

	UPROPERTY(EditAnywhere, DisplayName = "Rule Package", Category = "CGA")
	URulePackage* Rpk;

	UPROPERTY()
	bool bValidRandomSeed = false;

	UPROPERTY(EditAnywhere, Category = "CGA")
	int32 RandomSeed;

	UPROPERTY(EditAnywhere)
	TMap<FString, URuleAttribute*> Attributes;

	UPROPERTY(EditAnywhere)
	UMaterial* OpaqueParent;

	UPROPERTY(EditAnywhere)
	UMaterial* MaskedParent;

	UPROPERTY(EditAnywhere)
	UMaterial* TranslucentParent;

	UPROPERTY(EditAnywhere, Category = "CGA")
	bool GenerateAutomatically = true;

	UFUNCTION(BlueprintCallable)
	void Generate();

protected:
	void BeginPlay() override;

public:
	void Tick(float DeltaTime) override;

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	bool ShouldTickIfViewportsOnly() const override;
#endif

private:
	void LoadDefaultAttributes(UStaticMesh* InitialShape);
};
