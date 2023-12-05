/* Copyright 2023 Esri
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

#include "VitruvioModule.h"
#include "GenerateCompletedCallbackProxy.h"

#include "VitruvioBatchActor.generated.h"

UCLASS()
class UTile : public UObject
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TSet<UVitruvioComponent*> VitruvioComponents;

public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FIntPoint Location;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool bDirty;

	UPROPERTY()
	TMap<UVitruvioComponent*, UGenerateCompletedCallbackProxy*> CallbackProxies;

	FBatchGenerateResult::FTokenPtr GenerateToken;

	UPROPERTY()
	UGeneratedModelStaticMeshComponent* GeneratedModelComponent;
    	
	void MarkDirty(UVitruvioComponent* VitruvioComponent, UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);
	void UnmarkDirty();
	
	void Add(UVitruvioComponent* VitruvioComponent);
	void Remove(UVitruvioComponent* VitruvioComponent);
	bool Contains(UVitruvioComponent* VitruvioComponent) const;
	
	TArray<FInitialShape> GetInitialShapes();
	URulePackage* GetRpk();
	const TSet<UVitruvioComponent*>& GetVitruvioComponents();
};

USTRUCT()
struct FGrid
{
	GENERATED_BODY()

	TMap<FIntPoint, TMap<URulePackage*, UTile*>> Tiles;
	UPROPERTY()
	TMap<UVitruvioComponent*, UTile*> TilesByComponent;

	void MarkDirty(UVitruvioComponent* VitruvioComponent, UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);
	void RegisterAll(const TSet<UVitruvioComponent*>& VitruvioComponents, AVitruvioBatchActor* VitruvioBatchActor);
	void Register(UVitruvioComponent* VitruvioComponent, AVitruvioBatchActor* VitruvioBatchActor);
	void Unregister(UVitruvioComponent* VitruvioComponent);

	void Clear();

	TArray<UTile*> GetDirtyTiles() const;
	void UnmarkDirty();
};

struct FBatchGenerateQueueItem
{
	FGenerateResultDescription GenerateResultDescription;
	UTile* Tile;
};

UCLASS(NotBlueprintable, NotPlaceable)
class VITRUVIO_API AVitruvioBatchActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FIntVector2 GridDimension = {50000, 50000};

private:
	UPROPERTY(Transient)
	FGrid Grid;

	TQueue<FBatchGenerateQueueItem> GenerateQueue;

	UPROPERTY(Transient)
	TMap<UMaterialInterface*, FString> MaterialIdentifiers;
	TMap<FString, int32> UniqueMaterialIdentifiers;

	int NumModelComponents = 0;
	
	UPROPERTY(Transient)
	TSet<UVitruvioComponent*> VitruvioComponents;

	/** Default parent material for opaque geometry. */
	UPROPERTY(EditAnywhere, DisplayName = "Opaque Parent", Category = "Vitruvio Default Materials")
	UMaterial* OpaqueParent;

	/** Default parent material for masked geometry. */
	UPROPERTY(EditAnywhere, DisplayName = "Masked Parent", Category = "Vitruvio Default Materials")
	UMaterial* MaskedParent;

	UPROPERTY(EditAnywhere, DisplayName = "Translucent Parent", Category = "Vitruvio Default Materials")
	UMaterial* TranslucentParent;

public:
	AVitruvioBatchActor();

	virtual void Tick(float DeltaSeconds) override;

	void RegisterVitruvioComponent(UVitruvioComponent* VitruvioComponent);
	void UnregisterVitruvioComponent(UVitruvioComponent* VitruvioComponent);
	void MarkDirty(UVitruvioComponent* VitruvioComponent, UGenerateCompletedCallbackProxy* CallbackProxy = nullptr);

	FIntPoint GetPosition(const UVitruvioComponent* VitruvioComponent) const;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual bool ShouldTickIfViewportsOnly() const override;
	
private:
	void ProcessTiles();
	void ProcessGenerateQueue();

	FCriticalSection ProcessQueueCriticalSection;
};