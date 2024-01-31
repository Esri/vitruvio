// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#include "MaterialReplacementDialog.h"

#include "DetailLayoutBuilder.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "ISinglePropertyView.h"
#include "ReplacementDialog.h"
#include "VitruvioComponent.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"

void SMaterialReplacementDialogWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ReplacementDialogOptions);
}

void SMaterialReplacementDialogWidget::Construct(const FArguments& InArgs)
{
	ReplacementDialogOptions = NewObject<UMaterialReplacementDialogOptions>();
	ReplacementDialogOptions->TargetReplacementAsset = InArgs._VitruvioComponent->MaterialReplacement;

	// clang-format off
	SReplacementDialogWidget::Construct(SReplacementDialogWidget::FArguments()
		.ParentWindow(InArgs._ParentWindow)
		.VitruvioComponent(InArgs._VitruvioComponent)
		.GeneratedWithoutReplacements(InArgs._GeneratedWithoutReplacements));
	// clang-format on

	ApplyButton->SetEnabled(ReplacementDialogOptions->TargetReplacementAsset != nullptr);
}

FText SMaterialReplacementDialogWidget::CreateHeaderText()
{
	return FText::FromString(TEXT("Choose Material replacements and the DataTable where they will be added."));
}

TSharedPtr<ISinglePropertyView> SMaterialReplacementDialogWidget::CreateTargetReplacementWidget()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FSinglePropertyParams SinglePropertyArgs;
	SinglePropertyArgs.NamePlacement = EPropertyNamePlacement::Hidden;

	const TSharedPtr<ISinglePropertyView> TargetReplacementWidget = PropertyEditorModule.CreateSingleProperty(
		ReplacementDialogOptions, GET_MEMBER_NAME_CHECKED(UMaterialReplacementDialogOptions, TargetReplacementAsset), SinglePropertyArgs);

	return TargetReplacementWidget;
}

void SMaterialReplacementDialogWidget::UpdateApplyButtonEnablement()
{
	ApplyButton->SetEnabled(ReplacementDialogOptions->TargetReplacementAsset != nullptr);
}

void SMaterialReplacementDialogWidget::OnCreateNewAsset()
{
	CreateNewAsset<UMaterialReplacementAsset, UMaterialReplacementDialogOptions>(ReplacementDialogOptions);
}

void SMaterialReplacementDialogWidget::AddDialogOptions(const TSharedPtr<SVerticalBox>& Content)
{
	// clang-format off
	Content->AddSlot()
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
	];

	const FString ApplyToAllCheckBoxText = TEXT("Apply to all '") + VitruvioComponent->GetRpk()->GetName() + TEXT("' VitruvioActors");
	
	Content->AddSlot()
	.Padding(4)
	.AutoHeight()
	[
		SAssignNew(ApplyToAllVitruvioActorsCheckBox, SCheckBox)
		.IsChecked(true)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f))
			.Text(FText::FromString(ApplyToAllCheckBoxText))
		]
	];
	// clang-format on
}

void SMaterialReplacementDialogWidget::OnWindowClosed()
{
	for (const auto& [MaterialName, Replacement] : ReplacementDialogOptions->MaterialReplacements)
	{
		for (UStaticMeshComponent* StaticMeshComponent : Replacement->Components)
		{
			StaticMeshComponent->SetMaterialPreview(INDEX_NONE);
			StaticMeshComponent->SelectedEditorMaterial = INDEX_NONE;
		}
	}
}

void SMaterialReplacementDialogWidget::UpdateReplacementTable()
{
	ReplacementsBox->ClearChildren();
	IsolateCheckboxes.Empty();

	ReplacementDialogOptions->MaterialReplacements.Empty();

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	StaticMeshComponents.Add(VitruvioComponent->GetGeneratedModelComponent());

	if (IncludeInstancesCheckBox->IsChecked())
	{
		for (UGeneratedModelHISMComponent* HISMComponent : VitruvioComponent->GetGeneratedModelHISMComponents())
		{
			StaticMeshComponents.Add(HISMComponent);
		}
	}

	TMap<FString, UMaterialInterface*> CurrentReplacements;
	if (ReplacementDialogOptions->TargetReplacementAsset)
	{
		for (const FMaterialReplacementData& ReplacementData : ReplacementDialogOptions->TargetReplacementAsset->Replacements)
		{
			CurrentReplacements.Add(ReplacementData.MaterialIdentifier, {ReplacementData.ReplacementMaterial});
		}
	}

	for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		for (const auto& MaterialSlotName : StaticMeshComponent->GetMaterialSlotNames())
		{
			int32 MaterialIndex = StaticMeshComponent->GetMaterialIndex(MaterialSlotName);
			UMaterialInterface* SourceMaterial = StaticMeshComponent->GetMaterial(MaterialIndex);
			const FString& MaterialIdentifier = VitruvioComponent->GetMaterialIdentifier(SourceMaterial);

			if (auto ExistingReplacementOptional = ReplacementDialogOptions->MaterialReplacements.Find(MaterialIdentifier))
			{
				UMaterialReplacement* ExistingReplacement = *ExistingReplacementOptional;
				ExistingReplacement->Components.Add(StaticMeshComponent);
				ExistingReplacement->SourceMaterials.Add(SourceMaterial);
			}
			else
			{
				UMaterialReplacement* MaterialReplacement = NewObject<UMaterialReplacement>();
				if (UMaterialInterface** MaterialInterface = CurrentReplacements.Find(MaterialIdentifier))
				{
					MaterialReplacement->ReplacementMaterial = *MaterialInterface;
				}
				MaterialReplacement->MaterialIdentifier = MaterialIdentifier;
				MaterialReplacement->Components.Add(StaticMeshComponent);
				MaterialReplacement->SourceMaterials.Add(SourceMaterial);

				ReplacementDialogOptions->MaterialReplacements.Add(MaterialIdentifier, MaterialReplacement);
			}
		}
	}

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FSinglePropertyParams SinglePropertyArgs;
	SinglePropertyArgs.NamePlacement = EPropertyNamePlacement::Hidden;

	auto ResetMaterialPreview = [this](const TArray<TSharedPtr<SCheckBox>>& CheckBoxes, int32 IgnoreIndex) {
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
				StaticMeshComponent->SetVisibility(true, true);
				StaticMeshComponent->SetMaterialPreview(INDEX_NONE);
			}
		}
	};

	for (const auto& [Key, Replacement] : ReplacementDialogOptions->MaterialReplacements)
	{
		TSharedRef<SHorizontalBox> ReplacementBox = SNew(SHorizontalBox);
		TSharedPtr<STextBlock> SourceMaterialText;

		TSharedPtr<SCheckBox> IsolateCheckbox;

		TArray<FString> SourceMaterialNamesArray;
		Algo::Transform(Replacement->SourceMaterials, SourceMaterialNamesArray,
						[](const UMaterialInterface* MaterialInterface) { return MaterialInterface->GetName(); });
		FString SourceMaterialNames = FString::Join(SourceMaterialNamesArray, TEXT(", "));

		static const FSlateBrush* WarningBrush = FAppStyle::Get().GetBrush("Icons.AlertCircle");
		static const FName WarningColorStyle("Colors.AccentYellow");

		bool bIsDefaultMaterial = Replacement->MaterialIdentifier.StartsWith("CityEngineMaterial");
		
		// clang-format off
		ReplacementBox->AddSlot()
		.VAlign(VAlign_Top)
		.Padding(0, 8, 0,0)
		[
			SNew(SVerticalBox) +
			SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox) 
				
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 4 ,0)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(WarningBrush)
					.ColorAndOpacity(FAppStyle::Get().GetSlateColor(WarningColorStyle))
					.ToolTipText(FText::FromString("Materials with the default material name are discouraged from replacements as they are not unique. Consider setting an explicit material name in CGA using the material.name attribute."))
					.Visibility(bIsDefaultMaterial ? EVisibility::Visible : EVisibility::Collapsed)
				]
				
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SAssignNew(SourceMaterialText, STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.Text(FText::FromString(Key))
				]
			]

			+ SVerticalBox::Slot()
			.Padding(0, 4, 0, 0)
			.AutoHeight()
			[
				SAssignNew(SourceMaterialText, STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f))
				.Text(FText::FromString("[" + SourceMaterialNames + "]"))
			]
			
			+ SVerticalBox::Slot()
			.Padding(0, 4, 0, 0)
			.AutoHeight()
			[
				SAssignNew(IsolateCheckbox, SCheckBox)
				.OnCheckStateChanged_Lambda(
					[ResetMaterialPreview, Key, IgnoreIndex = IsolateCheckboxes.Num(), Replacement, this](ECheckBoxState CheckBoxState)
					{
						ResetMaterialPreview(IsolateCheckboxes, IgnoreIndex);

						for (const auto& [OtherKey, OtherReplacement] : ReplacementDialogOptions->MaterialReplacements)
						{
							const bool bVisible = (CheckBoxState == ECheckBoxState::Checked && Replacement == OtherReplacement) ||
													CheckBoxState == ECheckBoxState::Unchecked;
							for (UStaticMeshComponent* StaticMeshComponent : OtherReplacement->Components)
							{
								StaticMeshComponent->SetVisibility(bVisible, false);
							}
						}

						for (UStaticMeshComponent* StaticMeshComponent : Replacement->Components)
						{
							StaticMeshComponent->SetVisibility(true, false);

							for (int32 MaterialIndex = 0; MaterialIndex < StaticMeshComponent->GetNumMaterials(); ++MaterialIndex)
							{
								if (Replacement->SourceMaterials.Contains(StaticMeshComponent->GetMaterial(MaterialIndex)))
								{
									StaticMeshComponent->SetMaterialPreview(CheckBoxState == ECheckBoxState::Checked ? MaterialIndex : INDEX_NONE);
								}
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
		// clang-format on

		IsolateCheckboxes.Add(IsolateCheckbox);

		const TSharedPtr<ISinglePropertyView> SinglePropertyViewWidget = PropertyEditorModule.CreateSingleProperty(
			Replacement, GET_MEMBER_NAME_CHECKED(UMaterialReplacement, ReplacementMaterial), SinglePropertyArgs);

		// clang-format off
		ReplacementBox->AddSlot()
		[
			SNew(SBox).MinDesiredWidth(200)
			[
				SinglePropertyViewWidget.ToSharedRef()
			]
		];
		// clang-format on

		ReplacementsBox->AddSlot().Padding(4.0f).VAlign(VAlign_Fill).HAlign(HAlign_Fill)[ReplacementBox];
	}
}

FReply SMaterialReplacementDialogWidget::OnReplacementConfirmed()
{
	for (const auto& [MaterialName, Replacement] : ReplacementDialogOptions->MaterialReplacements)
	{
		for (UStaticMeshComponent* StaticMeshComponent : Replacement->Components)
		{
			StaticMeshComponent->SetMaterialPreview(INDEX_NONE);
			StaticMeshComponent->SelectedEditorMaterial = INDEX_NONE;
		}
	}

	if (ReplacementDialogOptions->TargetReplacementAsset)
	{
		for (const auto& Replacement : ReplacementDialogOptions->MaterialReplacements)
		{
			if (Replacement.Value->ReplacementMaterial)
			{
				if (OverrideExistingReplacements->IsChecked())
                {
                	ReplacementDialogOptions->TargetReplacementAsset->Replacements.RemoveAll([Replacement](const FMaterialReplacementData& MaterialReplacement)
                	{
                		return MaterialReplacement.MaterialIdentifier == Replacement.Value->MaterialIdentifier;
                	});
                }
                			
				FMaterialReplacementData MaterialReplacementData;
				MaterialReplacementData.MaterialIdentifier = Replacement.Value->MaterialIdentifier;
				MaterialReplacementData.ReplacementMaterial = Replacement.Value->ReplacementMaterial;
				ReplacementDialogOptions->TargetReplacementAsset->Replacements.Add(MaterialReplacementData);
			}
		}

		bReplacementsApplied = true;

		ReplacementDialogOptions->TargetReplacementAsset->MarkPackageDirty();
	}

	TArray<UVitruvioComponent*> ApplyToComponents = GetVitruvioActorsToApplyReplacements(ApplyToAllVitruvioActorsCheckBox->IsChecked());
	for (UVitruvioComponent* Component : ApplyToComponents)
	{
		Component->MaterialReplacement = ReplacementDialogOptions->TargetReplacementAsset;
		Component->Generate();
	}

	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply SMaterialReplacementDialogWidget::OnReplacementCanceled()
{
	for (const auto& [MaterialName, Replacement] : ReplacementDialogOptions->MaterialReplacements)
	{
		for (UStaticMeshComponent* StaticMeshComponent : Replacement->Components)
		{
			StaticMeshComponent->SetMaterialPreview(INDEX_NONE);
			StaticMeshComponent->SelectedEditorMaterial = INDEX_NONE;
		}
	}

	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}