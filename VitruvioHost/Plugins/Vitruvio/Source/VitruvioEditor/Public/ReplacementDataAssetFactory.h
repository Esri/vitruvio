#pragma once

#include "ReplacementDataAssetFactory.generated.h"

UCLASS()
class UReplacementDataAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = DataAsset)
	TSubclassOf<UDataAsset> DataAssetClass;

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context,
									  FFeedbackContext* Warn) override
	{
		check(DataAssetClass);

		return NewObject<UDataAsset>(InParent, DataAssetClass, Name, Flags | RF_Transactional);
	}
	// End of UFactory interface
};