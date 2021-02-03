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

#include "ChooseRulePackageDialog.h"
#include "RulePackage.h"

#include "PropertyCustomizationHelpers.h"
#include "PropertyEditor/Public/IPropertyTable.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SCompoundWidget.h"

class SCRulePackagePicker : public SCompoundWidget
{
	TWeakPtr<SWindow> WeakParentWindow;

	TSharedPtr<IPropertyTable> PropertyTable;
	TSharedPtr<IDetailsView> PickRpkDetailView;

	URulePackageOptions* RulePackageData = nullptr;

	bool bPressedOk = false;

public:
	SLATE_BEGIN_ARGS(SCRulePackagePicker) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	bool PressedOk() const
	{
		return bPressedOk;
	}

	URulePackage* GetPickedRpk() const
	{
		return RulePackageData ? RulePackageData->RulePackage : nullptr;
	}

private:
	FReply OnRpkPickerConfirmed();
	FReply OnRpkPickerCanceled();
};

void SCRulePackagePicker::Construct(const FArguments& InArgs)
{
	WeakParentWindow = InArgs._ParentWindow;

	RulePackageData = NewObject<URulePackageOptions>();

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bShowActorLabel = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bHideSelectionTip = false;
	DetailsViewArgs.bSearchInitialKeyFocus = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	PickRpkDetailView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	PickRpkDetailView->SetObject(RulePackageData);

	// clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(4.0f)
		.AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text(FText::FromString(TEXT("Choose a Rule Package which will be applied to all VitruvioComponents.")))
		]

		+ SVerticalBox::Slot()
		.Padding(4.0f)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			PickRpkDetailView->AsShared()
		]
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(2)[
			SNew(SUniformGridPanel).SlotPadding(2)
			+ SUniformGridPanel::Slot(0, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString("Apply"))
				.OnClicked(this, &SCRulePackagePicker::OnRpkPickerConfirmed)
			]

			+ SUniformGridPanel::Slot(1, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString("Cancel"))
				.OnClicked(this, &SCRulePackagePicker::OnRpkPickerCanceled)
			]
		]
	];
	// clang-format on
}

FReply SCRulePackagePicker::OnRpkPickerConfirmed()
{
	bPressedOk = true;

	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply SCRulePackagePicker::OnRpkPickerCanceled()
{
	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

TOptional<URulePackage*> FChooseRulePackageDialog::OpenDialog()
{
	// clang-format off
	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
	   .Title(FText::FromString("Choose Rule Package"))
	   .SizingRule(ESizingRule::UserSized)
	   .ClientSize(FVector2D(500.f, 300.f))
	   .SupportsMaximize(false)
	   .SupportsMinimize(false);
	// clang-format on

	TSharedRef<SCRulePackagePicker> RulePackagePicker = SNew(SCRulePackagePicker).ParentWindow(PickerWindow);
	PickerWindow->SetContent(RulePackagePicker);

	GEditor->EditorAddModalWindow(PickerWindow);

	if (RulePackagePicker->PressedOk())
	{
		return RulePackagePicker->GetPickedRpk();
	}
	return {};
}
