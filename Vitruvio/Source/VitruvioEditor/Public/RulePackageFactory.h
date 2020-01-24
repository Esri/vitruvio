#pragma once
#include "Factories/Factory.h"
#include "RulePackageFactory.generated.h"

UCLASS(hidecategories = Object)
class URulePackageFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer,
								 const uint8* BufferEnd, FFeedbackContext* Warn) override;

	bool FactoryCanImport(const FString& Filename) override;

	bool ConfigureProperties() override;
};