// Copyright © 2017-2022 Esri R&D Center Zurich. All rights reserved.

#include "MaterialReplacementDialog.h"

#include "DetailLayoutBuilder.h"
#include "ISinglePropertyView.h"
#include "VitruvioComponent.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"

UMaterialReplacementAsset* PreviousMaterialReplacementTarget = nullptr;

class SCMaterialReplacementPackagePicker : public SCompoundWidget
{
	TWeakPtr<SWindow> WeakParentWindow;
	UVitruvioComponent* VitruvioComponent = nullptr;
	UMaterialReplacementDialogOptions* ReplacementDialogOptions = nullptr;
	
	TSharedPtr<SScrollBox> ReplacementsBox;
	TArray<TSharedPtr<SCheckBox>> IsolateCheckboxes;
	TArray<TSharedPtr<SCheckBox>> HighlightCheckboxes;
	TSharedPtr<SCheckBox> IncludeInstancesCheckBox;
	
	bool bPressedOk = false;

public:
	SLATE_BEGIN_ARGS(SCMaterialReplacementPackagePicker) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_ARGUMENT(UVitruvioComponent*, VitruvioComponent)
	SLATE_END_ARGS()

	void UpdateReplacementTable();
	void Construct(const FArguments& InArgs);

	bool PressedOk() const
	{
		return bPressedOk;
	}

private:
	FReply OnReplacementConfirmed();
	FReply OnReplacementCanceled();
};

void SCMaterialReplacementPackagePicker::UpdateReplacementTable()
{
	ReplacementsBox->ClearChildren();
	IsolateCheckboxes.Empty();
	HighlightCheckboxes.Empty();
	
	ReplacementDialogOptions = NewObject<UMaterialReplacementDialogOptions>();
	ReplacementDialogOptions->TargetReplacementAsset = PreviousMaterialReplacementTarget;

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	StaticMeshComponents.Add(VitruvioComponent->GetGeneratedModelComponent());

	if (IncludeInstancesCheckBox->IsChecked())
	{
		for (UGeneratedModelHISMComponent* HISMComponent : VitruvioComponent->GetGeneratedModelHISMComponents())
		{
			StaticMeshComponents.Add(HISMComponent);
		}
	}

	for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		for (const auto& MaterialSlotName : StaticMeshComponent->GetMaterialSlotNames())
		{
			int32 MaterialIndex = StaticMeshComponent->GetMaterialIndex(MaterialSlotName);
			
			UMaterialInterface* SourceMaterial = StaticMeshComponent->GetMaterial(MaterialIndex);
			FMaterialKey Key { SourceMaterial, MaterialSlotName };
			if (auto ExistingReplacementOptional = ReplacementDialogOptions->MaterialReplacements.Find(Key))
			{
				UMaterialReplacement* ExistingReplacement = *ExistingReplacementOptional;
				ExistingReplacement->Components.Add(StaticMeshComponent);
			}
			else
			{
				UMaterialReplacement* MaterialReplacement = NewObject<UMaterialReplacement>();
				MaterialReplacement->Source = StaticMeshComponent->GetMaterial(MaterialIndex);
				MaterialReplacement->Components.Add(StaticMeshComponent);
				ReplacementDialogOptions->MaterialReplacements.Add({ SourceMaterial, MaterialSlotName }, MaterialReplacement);	
			}
		}
	}

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FSinglePropertyParams SinglePropertyArgs;
	SinglePropertyArgs.NamePlacement = EPropertyNamePlacement::Hidden;
	
	auto ResetMaterialPreview = [this](bool bHighlight, const TArray<TSharedPtr<SCheckBox>>& CheckBoxes, int32 IgnoreIndex)
	{
		for (int32 Index = 0; Index < CheckBoxes.Num(); ++Index)
		{
			if (Index != IgnoreIndex)
			{
				const TSharedPtr<SCheckBox>& CheckBox = CheckBoxes[Index];
				CheckBox->SetIsChecked(false);
			}
		}

		for (const auto& [Key, Replacement] : ReplacementDialogOptions->MaterialReplacements)
		{
			for (UStaticMeshComponent* StaticMeshComponent : Replacement->Components)
			{
				StaticMeshComponent->SetVisibility(false, true);
				
				if (bHighlight)
				{
					StaticMeshComponent->SetMaterialPreview(INDEX_NONE);
				}
				else
				{
					StaticMeshComponent->SelectedEditorMaterial = INDEX_NONE;
				}
			}
		}
	};
	
	for (const auto& [Key, Replacement] : ReplacementDialogOptions->MaterialReplacements)
	{
		TSharedRef<SHorizontalBox> ReplacementBox = SNew(SHorizontalBox);
		TSharedPtr<STextBlock> SourceMaterialText;

		TSharedPtr<SCheckBox> IsolateCheckbox;
		TSharedPtr<SCheckBox> HighlightCheckbox;

		TArray<FString> ComponentNamesArray;
		Algo::Transform(Replacement->Components, ComponentNamesArray, [](UStaticMeshComponent* Component) { return Component->GetName(); });
		FString ComponentNames = FString::Join(ComponentNamesArray, TEXT(", "));
		FString SourceMaterialAndComponentsText = Key.Material->GetName() + " [" + ComponentNames + "]";
		
		ReplacementBox->AddSlot()
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(SourceMaterialText, STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(FText::FromString(SourceMaterialAndComponentsText))
			]

			+ SVerticalBox::Slot()
			.Padding(0, 2, 0, 0)
			.AutoHeight()
			[
				SAssignNew(HighlightCheckbox, SCheckBox)
				.OnCheckStateChanged_Lambda([ResetMaterialPreview, SlotName = Key.SlotName, IgnoreIndex = HighlightCheckboxes.Num(), Replacement, this](ECheckBoxState CheckBoxState)
                {
                    ResetMaterialPreview(true, HighlightCheckboxes, IgnoreIndex);
					
                    if (CheckBoxState == ECheckBoxState::Checked)
                    {
                        for (UStaticMeshComponent* StaticMeshComponent : Replacement->Components)
                        {
                        	StaticMeshComponent->SetVisibility(true, true);
                            const int32 MaterialIndex = StaticMeshComponent->GetMaterialIndex(SlotName);
                        	StaticMeshComponent->SetMaterialPreview(MaterialIndex);
                        }
                    }
                })
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
				SAssignNew(IsolateCheckbox, SCheckBox)
				.OnCheckStateChanged_Lambda([ResetMaterialPreview, SlotName = Key.SlotName, IgnoreIndex = HighlightCheckboxes.Num(), Replacement, this](ECheckBoxState CheckBoxState)
				{
					ResetMaterialPreview(false, IsolateCheckboxes, IgnoreIndex);
					if (CheckBoxState == ECheckBoxState::Checked)
					{
						for (UStaticMeshComponent* StaticMeshComponent : Replacement->Components)
						{
							StaticMeshComponent->SetVisibility(true, true);
							const int32 MaterialIndex = StaticMeshComponent->GetMaterialIndex(SlotName);
							StaticMeshComponent->SelectedEditorMaterial = MaterialIndex;
						}
					}
				})
				.IsChecked(false)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f))
					.Text(FText::FromString("Isolate"))
				]
			]
		];

		IsolateCheckboxes.Add(IsolateCheckbox);
		HighlightCheckboxes.Add(HighlightCheckbox);

		const TSharedPtr<ISinglePropertyView> SinglePropertyViewWidget = PropertyEditorModule.CreateSingleProperty(Replacement, GET_MEMBER_NAME_CHECKED(UMaterialReplacement, Replacement), SinglePropertyArgs);
		ReplacementBox->AddSlot()
		[
			SNew(SBox)
			.MinDesiredWidth(200)
			[
				SinglePropertyViewWidget.ToSharedRef()
			]
		];
		
		ReplacementsBox->AddSlot()
        .Padding(4.0f)
        .VAlign(VAlign_Fill)
        .HAlign(HAlign_Fill)
		[
			ReplacementBox
		];
	}
}

void SCMaterialReplacementPackagePicker::Construct(const FArguments& InArgs)
{
	WeakParentWindow = InArgs._ParentWindow;
	VitruvioComponent = InArgs._VitruvioComponent;
	
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
		.Padding(4)
		.AutoHeight()
		[
			SAssignNew(IncludeInstancesCheckBox, SCheckBox)
			.OnCheckStateChanged_Lambda([this](ECheckBoxState CheckBoxState)
			{
				UpdateReplacementTable();
			})
			.IsChecked(true)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f))
				.Text(FText::FromString("Include Instances"))
			]
		]

		+ SVerticalBox::Slot()
		.Padding(4.0f)
		.AutoHeight()
		[
			SNew(SSeparator)
			.Orientation(Orient_Horizontal)
		]
		
		+ SVerticalBox::Slot()
		.Padding(4.0f)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SAssignNew(ReplacementsBox, SScrollBox)
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
	// clang-format on
	
	UpdateReplacementTable();
}

FReply SCMaterialReplacementPackagePicker::OnReplacementConfirmed()
{
	PreviousMaterialReplacementTarget = ReplacementDialogOptions->TargetReplacementAsset;
	bPressedOk = true;
	for (const auto& [MaterialName, Replacement] : ReplacementDialogOptions->MaterialReplacements)
	{
		for (UStaticMeshComponent* StaticMeshComponent : Replacement->Components)
		{
			StaticMeshComponent->SetMaterialPreview(INDEX_NONE);
			StaticMeshComponent->SelectedEditorMaterial = INDEX_NONE;
		}
	}
	
	// SourceMeshComponent->SetMaterialPreview(INDEX_NONE);
	
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
	// SourceMeshComponent->SetMaterialPreview(INDEX_NONE);
	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

void FMaterialReplacementDialog::OpenDialog(UVitruvioComponent* VitruvioComponent)
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
		.VitruvioComponent(VitruvioComponent)
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
