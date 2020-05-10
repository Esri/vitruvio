// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "RuleAttributes.h"
#include "RulePackage.h"

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/Material.h"

#include "PRTActor.generated.h"

// TODO: wondering if we should just call it "generate" instead of regenerate?
UCLASS()
class VITRUVIO_API APRTActor : public AStaticMeshActor
{
	GENERATED_BODY()

	bool Initialized = false;
	bool Regenerated = false;
	bool AttributesReady = false;

public:
	APRTActor();

	UPROPERTY(EditAnywhere, DisplayName = "Rule Package", Category = "CGA")
	URulePackage* Rpk;

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
	void Regenerate();

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
