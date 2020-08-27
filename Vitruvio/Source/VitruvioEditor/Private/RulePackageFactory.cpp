// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "RulePackageFactory.h"

#include "RulePackage.h"

URulePackageFactory::URulePackageFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SupportedClass = URulePackage::StaticClass();

	bCreateNew = false;
	bEditorImport = true;

	Formats.Add("rpk;Esri Rule Package");
}

// TODO: do we want to load the whole RPK into memory? Alternatively use FactoryCreateFile() and just keep the filename?
UObject* URulePackageFactory::FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context,
												  const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn)
{
	URulePackage* RulePackage = NewObject<URulePackage>(InParent, SupportedClass, Name, Flags | RF_Transactional);

	const SIZE_T DataSize = BufferEnd - Buffer;
	RulePackage->Data.Reset(DataSize);
	RulePackage->Data.AddUninitialized(DataSize);
	FMemory::Memcpy(RulePackage->Data.GetData(), Buffer, DataSize);

	return RulePackage;
}

bool URulePackageFactory::FactoryCanImport(const FString& Filename)
{
	return true;
}

bool URulePackageFactory::ConfigureProperties()
{
	return true;
}
