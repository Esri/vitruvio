// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#include "InstanceReplacementDialog.h"

#include "DetailLayoutBuilder.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "ISinglePropertyView.h"
#include "ReplacementDialog.h"
#include "CityEngineComponent.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"

void SInstanceReplacementDialogWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ReplacementDialogOptions);
}

void SInstanceReplacementDialogWidget::Construct(const FArguments& InArgs)
{
	ReplacementDialogOptions = NewObject<UInstanceReplacementDialogOptions>();
	ReplacementDialogOptions->TargetReplacementAsset = InArgs._VitruvioComponent->InstanceReplacement;

	// clang-format off
	SReplacementDialogWidget::Construct(SReplacementDialogWidget::FArguments()
		.ParentWindow(InArgs._ParentWindow)
		.VitruvioComponent(InArgs._VitruvioComponent)
		.GeneratedWithoutReplacements(InArgs._GeneratedWithoutReplacements));
	// clang-format on

	ApplyButton->SetEnabled(ReplacementDialogOptions->TargetReplacementAsset != nullptr);
}

FText SInstanceReplacementDialogWidget::CreateHeaderText()
{
	return FText::FromString(TEXT("Choose Instance replacements and the DataTable where they will be added."));
}

TSharedPtr<ISinglePropertyView> SInstanceReplacementDialogWidget::CreateTargetReplacementWidget()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FSinglePropertyParams SinglePropertyArgs;
	SinglePropertyArgs.NamePlacement = EPropertyNamePlacement::Hidden;

	const TSharedPtr<ISinglePropertyView> TargetReplacementWidget = PropertyEditorModule.CreateSingleProperty(
		ReplacementDialogOptions, GET_MEMBER_NAME_CHECKED(UInstanceReplacementDialogOptions, TargetReplacementAsset), SinglePropertyArgs);

	return TargetReplacementWidget;
}

void SInstanceReplacementDialogWidget::UpdateApplyButtonEnablement()
{
	ApplyButton->SetEnabled(ReplacementDialogOptions->TargetReplacementAsset != nullptr);
}

void SInstanceReplacementDialogWidget::OnCreateNewAsset()
{
	CreateNewAsset<UInstanceReplacementAsset, UInstanceReplacementDialogOptions>(ReplacementDialogOptions);
}

void SInstanceReplacementDialogWidget::AddDialogOptions(const TSharedPtr<SVerticalBox>& Content)
{
	// clang-format off
	const FString ApplyToAllCheckBoxText = TEXT("Apply to all '") + CityEngineComponent->GetRpk()->GetName() + TEXT("' VitruvioActors");
	
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

void SInstanceReplacementDialogWidget::OnWindowClosed()
{
	for (const auto& [InstanceKey, Replacement] : ReplacementDialogOptions->InstanceReplacements)
	{
		if (CityEngineComponent->GetGeneratedModelComponent())
		{
			CityEngineComponent->GetGeneratedModelComponent()->SetVisibility(true, false);
		}

		for (UStaticMeshComponent* MeshComponent : Replacement->MeshComponents)
		{
			MeshComponent->SetVisibility(true, false);
		}
	}
}

void SInstanceReplacementDialogWidget::UpdateReplacementTable()
{
	ReplacementsBox->ClearChildren();
	IsolateCheckboxes.Empty();

	ReplacementDialogOptions->InstanceReplacements.Empty();

	TMap<FString, FInstanceReplacement> CurrentReplacements;
	if (ReplacementDialogOptions->TargetReplacementAsset)
	{
		for (const FInstanceReplacement& ReplacementData : ReplacementDialogOptions->TargetReplacementAsset->Replacements)
		{
			CurrentReplacements.Add(ReplacementData.SourceMeshIdentifier, ReplacementData);
		}
	}

	for (UGeneratedModelHISMComponent* HISMComponent : CityEngineComponent->GetGeneratedModelHISMComponents())
	{
		UInstanceReplacementWrapper* InstanceReplacement;
		if (UInstanceReplacementWrapper** Existing = ReplacementDialogOptions->InstanceReplacements.Find(HISMComponent->GetMeshIdentifier()))
		{
			InstanceReplacement = *Existing;
		}
		else
		{
			InstanceReplacement = NewObject<UInstanceReplacementWrapper>();
			InstanceReplacement->SourceMeshIdentifier = HISMComponent->GetMeshIdentifier();
			ReplacementDialogOptions->InstanceReplacements.Add(HISMComponent->GetMeshIdentifier(), InstanceReplacement);

			if (FInstanceReplacement* Replacement = CurrentReplacements.Find(HISMComponent->GetMeshIdentifier()))
			{
				InstanceReplacement->Replacements = Replacement->Replacements;
			}
		}

		InstanceReplacement->MeshComponents.Add(HISMComponent);
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
	};

	for (const auto& [Key, Replacement] : ReplacementDialogOptions->InstanceReplacements)
	{
		TSharedRef<SHorizontalBox> ReplacementBox = SNew(SHorizontalBox);
		TSharedPtr<STextBlock> SourceMaterialText;

		TSharedPtr<SCheckBox> IsolateCheckbox;

		FString MeshIdentifier = Replacement->SourceMeshIdentifier;

		TArray<FString> MeshNamesArray;
		Algo::Transform(Replacement->MeshComponents, MeshNamesArray,
						[](const UStaticMeshComponent* StaticMeshComponent) { return StaticMeshComponent->GetName(); });

		const FString MeshNameString = FString::Join(MeshNamesArray, TEXT(", "));

		// clang-format off
		ReplacementBox->AddSlot()
		.VAlign(VAlign_Top)
		.Padding(0, 8, 0,0)
		[
			SNew(SVerticalBox) +
			SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(SourceMaterialText, STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(FText::FromString(MeshIdentifier))
			]

			+ SVerticalBox::Slot()
			.Padding(0, 4, 0, 0)
			.AutoHeight()
			[
				SAssignNew(SourceMaterialText, STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f))
				.Text(FText::FromString("[" + MeshNameString + "]"))
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

						CityEngineComponent->GetGeneratedModelComponent()->SetVisibility(CheckBoxState != ECheckBoxState::Checked, false);
						
						for (const auto& [OtherKey, OtherReplacement] : ReplacementDialogOptions->InstanceReplacements)
						{
							const bool bVisible = (CheckBoxState == ECheckBoxState::Checked && Replacement == OtherReplacement) ||
													CheckBoxState == ECheckBoxState::Unchecked;

							for (UStaticMeshComponent* MeshComponent : OtherReplacement->MeshComponents)
							{
								MeshComponent->SetVisibility(bVisible, false);
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

		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bShowObjectLabel = false;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowCustomFilterOption = false;
		DetailsViewArgs.bShowScrollBar = false;
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bShowSectionSelector = false;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;

		TSharedRef<IDetailsView> MeshReplacementsDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

		MeshReplacementsDetailsView->SetObject(Replacement, true);

		// clang-format off
		ReplacementBox->AddSlot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(4)
			[
				SNew(SBox)
				.MinDesiredWidth(200)
				[
					MeshReplacementsDetailsView
				]
			]
		];

		// clang-format on

		ReplacementsBox->AddSlot().Padding(4.0f).VAlign(VAlign_Fill).HAlign(HAlign_Fill)[ReplacementBox];
	}
}

FReply SInstanceReplacementDialogWidget::OnReplacementConfirmed()
{
	if (ReplacementDialogOptions->TargetReplacementAsset)
	{
		for (const auto& Replacement : ReplacementDialogOptions->InstanceReplacements)
		{
			if (Replacement.Value->Replacements.IsEmpty())
			{
				continue;
			}

			if (OverrideExistingReplacements->IsChecked())
			{
				ReplacementDialogOptions->TargetReplacementAsset->Replacements.RemoveAll([Replacement](const FInstanceReplacement& InstanceReplacement)
				{
					return InstanceReplacement.SourceMeshIdentifier == Replacement.Value->SourceMeshIdentifier;
				});
			}

			FInstanceReplacement InstanceReplacement;
			InstanceReplacement.SourceMeshIdentifier = Replacement.Value->SourceMeshIdentifier;
			InstanceReplacement.Replacements = Replacement.Value->Replacements;
			ReplacementDialogOptions->TargetReplacementAsset->Replacements.Add(InstanceReplacement);
		}

		bReplacementsApplied = true;

		ReplacementDialogOptions->TargetReplacementAsset->MarkPackageDirty();
	}

	TArray<UCityEngineComponent*> ApplyToComponents = GetCityEngineActorsToApplyReplacements(ApplyToAllVitruvioActorsCheckBox->IsChecked());
	for (UCityEngineComponent* Component : ApplyToComponents)
	{
		Component->InstanceReplacement = ReplacementDialogOptions->TargetReplacementAsset;
		Component->Generate();
	}

	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply SInstanceReplacementDialogWidget::OnReplacementCanceled()
{
	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}
