#pragma once

#include "CoreMinimal.h"
#include "RulePackage.h"

#include "ChooseRulePackageDialog.generated.h"

UCLASS(meta = (DisplayName = "Options"))
class URulePackageOptions : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Vitruvio")
	URulePackage* RulePackage;
};

class FChooseRulePackageDialog
{
public:
	static TOptional<URulePackage*> OpenDialog();
};
