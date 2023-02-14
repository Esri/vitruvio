// Copyright © 2017-2022 Esri R&D Center Zurich. All rights reserved.

#include "MaterialReplacementDialog.h"

#include "DetailLayoutBuilder.h"
#include "ISinglePropertyView.h"
#include "Selection.h"
#include "Components/SinglePropertyView.h"
#include "Components/SizeBox.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"

UMaterialReplacementAsset* PreviousMaterialReplacementTarget = nullptr;

class SCMaterialReplacementPackagePicker : public SCompoundWidget
{
	TWeakPtr<SWindow> WeakParentWindow;
	UStaticMeshComponent* SourceMeshComponent = nullptr;
	UMaterialReplacementDialogOptions* ReplacementDialogOptions = nullptr;

	bool bPressedOk = false;

public:
	SLATE_BEGIN_ARGS(SCMaterialReplacementPackagePicker) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_ARGUMENT(UStaticMeshComponent*, SourceMeshComponent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	bool PressedOk() const
	{
		return bPressedOk;
	}

private:
	FReply OnReplacementConfirmed();
	FReply OnReplacementCanceled();
};

void SCMaterialReplacementPackagePicker::Construct(const FArguments& InArgs)
{
	WeakParentWindow = InArgs._ParentWindow;
	SourceMeshComponent = InArgs._SourceMeshComponent;
	ReplacementDialogOptions = NewObject<UMaterialReplacementDialogOptions>();
	ReplacementDialogOptions->TargetReplacementAsset = PreviousMaterialReplacementTarget;

	
	for (const auto& MaterialSlotName : InArgs._SourceMeshComponent->GetMaterialSlotNames())
	{
		int32 MaterialIndex = InArgs._SourceMeshComponent->GetMaterialIndex(MaterialSlotName);
		UMaterialReplacement* MaterialReplacement = NewObject<UMaterialReplacement>();
		MaterialReplacement->Source = InArgs._SourceMeshComponent->GetMaterial(MaterialIndex);
		ReplacementDialogOptions->MaterialReplacements.Add(MaterialSlotName, MaterialReplacement);	
	}

	TSharedPtr<SScrollBox> ReplacementVerticalBox;
	
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
			.Text(FText::FromString(TEXT("Choose a replacement Material and the DataTable where the Replacement will be added.")))
		]

		+ SVerticalBox::Slot()
		.Padding(4.0f)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SAssignNew(ReplacementVerticalBox, SScrollBox)
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
				.OnClicked(this, &SCMaterialReplacementPackagePicker::OnReplacementConfirmed)
			]

			+ SUniformGridPanel::Slot(1, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString("Cancel"))
				.OnClicked(this, &SCMaterialReplacementPackagePicker::OnReplacementCanceled)
			]
		]
	];

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FSinglePropertyParams SinglePropertyArgs;
	SinglePropertyArgs.NamePlacement = EPropertyNamePlacement::Hidden;

	const TArray<FName> MaterialSlotNames = SourceMeshComponent->GetMaterialSlotNames();
	for (const FName& SlotName : MaterialSlotNames)
	{
		int32 MaterialIndex = SourceMeshComponent->GetMaterialIndex(SlotName);
		UMaterialInterface* Material = SourceMeshComponent->GetMaterial(MaterialIndex);

		TSharedRef<SHorizontalBox> ReplacementBox = SNew(SHorizontalBox);

		TSharedPtr<STextBlock> SourceMaterial;
		ReplacementBox->AddSlot()
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(SourceMaterial, STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(FText::FromString(SlotName.ToString()))
			]

			+ SVerticalBox::Slot()
			.Padding(0, 2, 0, 0)
			.AutoHeight()
			[
				SNew(SCheckBox)
				.IsChecked(false)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f))
					.Text(FText::FromString("Highlight"))
				]
			]

			+ SVerticalBox::Slot()
			.Padding(0, 2, 0, 0)
			.AutoHeight()
			[
				SNew(SCheckBox)
				.IsChecked(false)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f))
					.Text(FText::FromString("Isolate"))
				]
			]
		];

		auto MaterialReplacement = ReplacementDialogOptions->MaterialReplacements.FindChecked(SlotName);
		const TSharedPtr<ISinglePropertyView> SinglePropertyViewWidget = PropertyEditorModule.CreateSingleProperty(MaterialReplacement, GET_MEMBER_NAME_CHECKED(UMaterialReplacement, Replacement), SinglePropertyArgs);
		ReplacementBox->AddSlot()
		[
			SNew(SBox)
			.MinDesiredWidth(200) [
				SinglePropertyViewWidget.ToSharedRef()
			]
		];
		
		ReplacementVerticalBox->AddSlot()
		.Padding(4.0f)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			ReplacementBox
		];
	}
	// clang-format on
}

FReply SCMaterialReplacementPackagePicker::OnReplacementConfirmed()
{
	PreviousMaterialReplacementTarget = ReplacementDialogOptions->TargetReplacementAsset;
	bPressedOk = true;
	SourceMeshComponent->SetMaterialPreview(INDEX_NONE);
	
	if (ReplacementDialogOptions->TargetReplacementAsset)
	{
		// ReplacementDialogOptions->TargetReplacementAsset->Replacements.Add(MaterialReplacementData);
	}

	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply SCMaterialReplacementPackagePicker::OnReplacementCanceled()
{
	SourceMeshComponent->SetMaterialPreview(INDEX_NONE);
	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

void FMaterialReplacementDialog::OpenDialog(UStaticMeshComponent* SourceMeshComponent)
{
	// clang-format off
	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
	   .Title(FText::FromString("Choose Replacement"))
	   .SizingRule(ESizingRule::UserSized)
	   .ClientSize(FVector2D(500.f, 400.f))
	   .IsTopmostWindow(true)
	   .SupportsMaximize(false)
	   .SupportsMinimize(false);
	// clang-format on

	TSharedRef<SCMaterialReplacementPackagePicker> ReplacementPicker = SNew(SCMaterialReplacementPackagePicker)
		.SourceMeshComponent(SourceMeshComponent)
		.ParentWindow(PickerWindow);
	PickerWindow->SetContent(ReplacementPicker);

	TSharedPtr<SWindow> ParentWindow = FGlobalTabmanager::Get()->GetRootWindow();
	if (ParentWindow.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(PickerWindow, ParentWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(PickerWindow);
	}
}
