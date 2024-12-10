/* Copyright 2024 Esri
 *
 * Licensed under the Apache License Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "CoreMinimal.h"

#include "CityEngineModule.h"
#include "GenerateCompletedCallbackProxy.h"

#include "CityEngineBatchActor.generated.h"

UCLASS()
class UTile : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "CityEngine")
	TSet<UCityEngineComponent*> CityEngineComponents;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "CityEngine")
	FIntPoint Location;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "CityEngine")
	bool bMarkedForGenerate;
	bool bIsGenerating;

	UPROPERTY()
	TMap<UCityEngineComponent*, UGenerateCompletedCallbackProxy*> CallbackProxies;

	FBatchGenerateResult::FTokenPtr GenerateToken;

	UPROPERTY()
	UGeneratedModelStaticMeshComponent* GeneratedModelComponent;
    	
	void MarkForGenerate(UCityEngineComponent* CityEngineComponent, UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);
	void UnmarkForGenerate();
	
	void Add(UCityEngineComponent* CityEngineComponent);
	void Remove(UCityEngineComponent* CityEngineComponent);
	bool Contains(UCityEngineComponent* CityEngineComponent) const;
	
	TTuple<TArray<FInitialShape>, TArray<UCityEngineComponent*>> GetInitialShapes();
};

USTRUCT()
struct FGrid
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FIntPoint, UTile*> Tiles;
	UPROPERTY()
	TMap<UCityEngineComponent*, UTile*> TilesByComponent;

	void MarkForGenerate(UCityEngineComponent* CityEngineComponent, UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);
	void MarkAllForGenerate();
	
	void RegisterAll(const TSet<UCityEngineComponent*>& CityEngineComponents, ACityEngineBatchActor* CityEngineBatchActor);
	void Register(UCityEngineComponent* CityEngineComponent, ACityEngineBatchActor* CityEngineBatchActor);
	void Unregister(UCityEngineComponent* CityEngineComponent);

	void Clear();

	TArray<UTile*> GetTilesMarkedForGenerate() const;
	void UnmarkForGenerate();
};

struct FBatchGenerateQueueItem
{
	FGenerateResultDescription GenerateResultDescription;
	UTile* Tile;
	TArray<UCityEngineComponent*> CityEngineComponents;
};

UCLASS(NotBlueprintable, NotPlaceable)
class CITYENGINEPLUGIN_API ACityEngineBatchActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "CityEngine")
	FIntVector2 GridDimension = {50000, 50000};

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "CityEngine")
	bool bDebugVisualizeGrid = false;
#endif
	
private:
	UPROPERTY(Transient)
	FGrid Grid;

	TQueue<FBatchGenerateQueueItem> GenerateQueue;

	UPROPERTY(Transient)
	TMap<UMaterialInterface*, FString> MaterialIdentifiers;
	TMap<FString, int32> UniqueMaterialIdentifiers;

	int NumModelComponents = 0;
	
	UPROPERTY(Transient)
	TSet<UCityEngineComponent*> CityEngineComponents;

	/** Default parent material for opaque geometry. */
	UPROPERTY(EditAnywhere, DisplayName = "Opaque Parent", Category = "CityEngine Default Materials")
	UMaterial* OpaqueParent;

	/** Default parent material for masked geometry. */
	UPROPERTY(EditAnywhere, DisplayName = "Masked Parent", Category = "CityEngine Default Materials")
	UMaterial* MaskedParent;

	UPROPERTY(EditAnywhere, DisplayName = "Translucent Parent", Category = "CityEngine Default Materials")
	UMaterial* TranslucentParent;

	/** The material replacement asset which defines how materials are replaced after generating a model. */
	UPROPERTY(EditAnywhere, Category = "CityEngine Replacmeents", Setter = SetMaterialReplacementAsset)
	UMaterialReplacementAsset* MaterialReplacement;

	/** The instance replacement asset which defines how instances are replaced after generating a model. */
	UPROPERTY(EditAnywhere, Category = "CityEngine Replacmeents", Setter = SetInstanceReplacementAsset)
	UInstanceReplacementAsset* InstanceReplacement;

public:
	ACityEngineBatchActor();

	virtual void Tick(float DeltaSeconds) override;

	void RegisterCityEngineComponent(UCityEngineComponent* CityEngineComponent);
	void UnregisterCityEngineComponent(UCityEngineComponent* CityEngineComponent);
	void UnregisterAllCityEngineComponents();
	TSet<UCityEngineComponent*> GetCityEngineComponents();

	void Generate(UCityEngineComponent* CityEngineComponent, UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);
	void GenerateAll(UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);
	
	FIntPoint GetPosition(const UCityEngineComponent* CityEngineComponent) const;
	
#if WITH_EDITOR
	virtual bool CanDeleteSelectedActor(FText& OutReason) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual bool ShouldTickIfViewportsOnly() const override;

	/**
	 * Sets the material replacement Asset and regenerates the model.
	 */
	UFUNCTION(BlueprintCallable, Category = "CityEngine Replacmeents")
	void SetMaterialReplacementAsset(UMaterialReplacementAsset* MaterialReplacementAsset);

	/**
	 * Sets the instance replacement Asset and regenerates the model.
	 */
	UFUNCTION(BlueprintCallable, Category = "CityEngine Replacmeents")
	void SetInstanceReplacementAsset(UInstanceReplacementAsset* InstanceReplacementAsset);

	
private:
	void ProcessTiles();
	void ProcessGenerateQueue();

	FCriticalSection ProcessQueueCriticalSection;

	UPROPERTY()
	UGenerateCompletedCallbackProxy* GenerateAllCallbackProxy;
};
