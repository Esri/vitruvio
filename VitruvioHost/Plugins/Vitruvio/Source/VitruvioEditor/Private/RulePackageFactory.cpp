/* Copyright 2021 Esri
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
