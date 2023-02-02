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

#include "RulePackageFactory.h"

#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"
#include "RulePackage.h"

URulePackageFactory::URulePackageFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SupportedClass = URulePackage::StaticClass();

	bCreateNew = false;
	bEditorImport = true;

	Formats.Add("rpk;Esri Rule Package");
}

bool URulePackageFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	URulePackage* RulePackage = Cast<URulePackage>(Obj);
	if (RulePackage)
	{
		OutFilenames.Add(UAssetImportData::ResolveImportFilename(RulePackage->SourcePath, RulePackage->GetOutermost()));
		return true;
	}
	return false;
}

void URulePackageFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	URulePackage* RulePackage = Cast<URulePackage>(Obj);
	if (RulePackage && ensure(NewReimportPaths.Num() == 1))
	{
		RulePackage->SourcePath = UAssetImportData::SanitizeImportFilename(NewReimportPaths[0], RulePackage->GetOutermost());
	}
}

EReimportResult::Type URulePackageFactory::Reimport(UObject* Obj)
{
	URulePackage* RulePackage = Cast<URulePackage>(Obj);
	if (!RulePackage)
	{
		return EReimportResult::Failed;
	}

	const FString Filename = UAssetImportData::ResolveImportFilename(RulePackage->SourcePath, RulePackage->GetOutermost());
	if (!FPaths::GetExtension(Filename).Equals(TEXT("rpk")))
	{
		return EReimportResult::Failed;
	}

	CurrentFilename = Filename;
	TArray<uint8> Data;
	if (FFileHelper::LoadFileToArray(Data, *Filename))
	{
		RulePackage->Modify();
		RulePackage->MarkPackageDirty();

		RulePackage->Data = Data;
		RulePackage->SourcePath = UAssetImportData::SanitizeImportFilename(CurrentFilename, RulePackage->GetOutermost());
	}

	return EReimportResult::Succeeded;
}

UObject* URulePackageFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename,
												const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{

	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *Filename))
	{
		return nullptr;
	}

	URulePackage* RulePackage = NewObject<URulePackage>(InParent, SupportedClass, InName, Flags | RF_Transactional);
	RulePackage->Data = Data;
	RulePackage->SourcePath = UAssetImportData::ResolveImportFilename(Filename, RulePackage->GetOutermost());
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
