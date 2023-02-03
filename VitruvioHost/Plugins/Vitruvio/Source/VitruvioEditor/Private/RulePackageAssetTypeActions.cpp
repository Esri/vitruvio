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

#include "RulePackageAssetTypeActions.h"

#include "ToolMenuSection.h"
#include "UnrealEd.h"

FText FRulePackageAssetTypeActions::GetName() const
{
	return FText::FromString("Esri Rule Package");
}

FColor FRulePackageAssetTypeActions::GetTypeColor() const
{
	return FColor(255, 0, 0);
}

UClass* FRulePackageAssetTypeActions::GetSupportedClass() const
{
	return URulePackage::StaticClass();
}

void FRulePackageAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor) {}

uint32 FRulePackageAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

bool FRulePackageAssetTypeActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

void FRulePackageAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
{
	TArray<TWeakObjectPtr<URulePackage>> RulePackages = GetTypedWeakObjectPtrs<URulePackage>(InObjects);

	Section.AddMenuEntry(
		"Rulpackage_Reload", FText::FromString("Reload From Disk"), FText::FromString("Reloads the Rule Package from disk."), FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FRulePackageAssetTypeActions::ExecuteReimport, RulePackages), FCanExecuteAction()));
}

void FRulePackageAssetTypeActions::ExecuteReimport(TArray<TWeakObjectPtr<URulePackage>> RulePackages)
{
	for (const auto& RulePackage : RulePackages)
	{
		if (RulePackage.IsValid())
		{
			FReimportManager::Instance()->Reimport(RulePackage.Get(), true);
		}
	}
}
