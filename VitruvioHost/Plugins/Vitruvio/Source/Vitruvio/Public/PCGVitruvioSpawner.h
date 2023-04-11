#pragma once

#include "PCGContext.h"
#include "PCGSettings.h"

#include "Engine/CollisionProfile.h"

#include "PCGPin.h"
#include "RulePackage.h"

#include "PCGVitruvioSpawner.generated.h"

class UPCGSpatialData;
struct FPCGPackedCustomData;

UCLASS(BlueprintType, ClassGroup = (Procedural))
class VITRUVIO_API UPCGVitruvioSpawnerSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGVitruvioSpawnerSettings(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	// ~Begin UPCGSettings interface
	virtual FName GetDefaultNodeName() const override
	{
		return FName(TEXT("VitruvioSpawner"));
	}
	virtual EPCGSettingsType GetType() const override
	{
		return EPCGSettingsType::Spawner;
	}
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override
	{
		return Super::DefaultPointInputPinProperties();
	}
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override
	{
		return Super::DefaultPointOutputPinProperties();
	}

	virtual FPCGElementPtr CreateElement() const override;
	// ~End UPCGSettings interface

public:
	// ~Begin UObject interface
	virtual void PostLoad() override;

	/** CityEngine Rule Package used for generation. */
	UPROPERTY(EditAnywhere, DisplayName = "Rule Package", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URulePackage> Rpk;

	/** Attribute name to store mesh SoftObjectPaths inside if the output pin is connected. Note: Will overwrite existing data if the attribute name
	 * already exists. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	FName OutAttributeName = NAME_None;
};

USTRUCT()
struct FPCGVitruvioSpawnerContext : public FPCGContext
{
	GENERATED_BODY()

	int32 CurrentDataIndex = 0;
};

class FPCGVitruvioSpawnerElement : public IPCGElement
{
public:
	virtual FPCGContext* Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent,
									const UPCGNode* Node) override;
	virtual bool CanExecuteOnlyOnMainThread(FPCGContext* Context) const;
	virtual bool IsCacheable(const UPCGSettings* InSettings) const override
	{
		return false;
	}

protected:
	virtual bool PrepareDataInternal(FPCGContext* Context) const override;
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};

#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#include "CoreMinimal.h"
#include "InstancePackers/PCGInstancePackerBase.h"
#include "MeshSelectors/PCGMeshSelectorBase.h"
#endif
