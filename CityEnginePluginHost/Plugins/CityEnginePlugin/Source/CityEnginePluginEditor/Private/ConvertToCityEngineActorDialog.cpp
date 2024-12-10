/* Copyright 2024 Esri
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

#include "ConvertToCityEngineActorDialog.h"

#include "Editor/PropertyEditor/Public/IPropertyTable.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SCompoundWidget.h"

class SCConvertOptionsWidget : public SCompoundWidget
{
	TWeakPtr<SWindow> WeakParentWindow;

	TSharedPtr<IPropertyTable> PropertyTable;
	TSharedPtr<IDetailsView> PickRpkDetailView;

	UConvertOptions* Options = nullptr;

	bool bPressedOk = false;

public:
	SLATE_BEGIN_ARGS(SCConvertOptionsWidget) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	bool PressedOk() const
	{
		return bPressedOk;
	}

	UConvertOptions* GetOptions() const
	{
		return Options;
	}

private:
	FReply OnConfirmed();
	FReply OnCanceled();
};

void SCConvertOptionsWidget::Construct(const FArguments& InArgs)
{
	WeakParentWindow = InArgs._ParentWindow;

	Options = NewObject<UConvertOptions>();

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bShowObjectLabel = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bHideSelectionTip = false;
	DetailsViewArgs.bSearchInitialKeyFocus = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	PickRpkDetailView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	PickRpkDetailView->SetObject(Options);

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
				.OnClicked(this, &SCConvertOptionsWidget::OnConfirmed)
			]

			+ SUniformGridPanel::Slot(1, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString("Cancel"))
				.OnClicked(this, &SCConvertOptionsWidget::OnCanceled)
			]
		]
	];
	// clang-format on
}

FReply SCConvertOptionsWidget::OnConfirmed()
{
	bPressedOk = true;

	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply SCConvertOptionsWidget::OnCanceled()
{
	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

TOptional<UConvertOptions*> FConvertToCityEngineActorDialog::OpenDialog()
{
	// clang-format off
	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
	   .Title(FText::FromString("Choose Rule Package"))
	   .SizingRule(ESizingRule::UserSized)
	   .ClientSize(FVector2D(500.f, 300.f))
	   .SupportsMaximize(false)
	   .SupportsMinimize(false);
	// clang-format on

	TSharedRef<SCConvertOptionsWidget> RulePackagePicker = SNew(SCConvertOptionsWidget).ParentWindow(PickerWindow);
	PickerWindow->SetContent(RulePackagePicker);

	GEditor->EditorAddModalWindow(PickerWindow);

	if (RulePackagePicker->PressedOk())
	{
		return RulePackagePicker->GetOptions();
	}
	return {};
}
