#include "ReplacementDataAssetFactory.h"

UReplacementDataAssetFactory::UReplacementDataAssetFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UDataAsset::StaticClass();
}
