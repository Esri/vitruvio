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

class SMaterialReplacementDialogWidget : public SReplacementDialogWidget
{
	UMaterialReplacementDialogOptions* ReplacementDialogOptions = nullptr;

	TArray<TSharedPtr<SCheckBox>> IsolateCheckboxes;
	TSharedPtr<SCheckBox> IncludeInstancesCheckBox;
	TSharedPtr<SCheckBox> ApplyToAllVitruvioActorsCheckBox;

public:
	SLATE_BEGIN_ARGS(SMaterialReplacementDialogWidget) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_ARGUMENT(UVitruvioComponent*, VitruvioComponent)
	SLATE_END_ARGS()

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	void Construct(const FArguments& InArgs);

protected:
	virtual FText CreateHeaderText() override;
	virtual TSharedPtr<ISinglePropertyView> CreateTargetReplacementWidget() override;
	virtual void UpdateApplyButtonEnablement() override;
	virtual void OnCreateNewAsset() override;
	virtual void AddDialogOptions(const TSharedPtr<SVerticalBox>& Content) override;
	virtual void OnWindowClosed() override;
	virtual void UpdateReplacementTable() override;
	virtual FReply OnReplacementConfirmed() override;
	virtual FReply OnReplacementCanceled() override;
};

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
		.VitruvioComponent(InArgs._VitruvioComponent));
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
	.Padding(4, 12, 4, 4)
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

	VitruvioComponent->Generate();
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

	TMap<FName, UMaterialInterface*> CurrentReplacements;
	if (ReplacementDialogOptions->TargetReplacementAsset)
	{
		for (const FMaterialReplacementData& ReplacementData : ReplacementDialogOptions->TargetReplacementAsset->Replacements)
		{
			CurrentReplacements.Add(ReplacementData.SourceMaterialSlotName, ReplacementData.ReplacementMaterial);
		}
	}

	for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		for (const auto& MaterialSlotName : StaticMeshComponent->GetMaterialSlotNames())
		{
			int32 MaterialIndex = StaticMeshComponent->GetMaterialIndex(MaterialSlotName);

			UMaterialInterface* SourceMaterial = StaticMeshComponent->GetMaterial(MaterialIndex);

			FMaterialKey Key{SourceMaterial, MaterialSlotName};
			if (auto ExistingReplacementOptional = ReplacementDialogOptions->MaterialReplacements.Find(Key))
			{
				UMaterialReplacement* ExistingReplacement = *ExistingReplacementOptional;
				ExistingReplacement->Components.Add(StaticMeshComponent);
			}
			else
			{
				UMaterialReplacement* MaterialReplacement = NewObject<UMaterialReplacement>();
				if (UMaterialInterface** MaterialInterface = CurrentReplacements.Find(MaterialSlotName))
				{
					MaterialReplacement->ReplacementMaterial = *MaterialInterface;
				}
				MaterialReplacement->SourceMaterialSlot = MaterialSlotName;
				MaterialReplacement->Components.Add(StaticMeshComponent);
				ReplacementDialogOptions->MaterialReplacements.Add({SourceMaterial, MaterialSlotName}, MaterialReplacement);
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

		TArray<FString> ComponentNamesArray;
		Algo::Transform(Replacement->Components, ComponentNamesArray, [](UStaticMeshComponent* Component) { return Component->GetName(); });
		FString ComponentNames = FString::Join(ComponentNamesArray, TEXT(", "));
		FString SourceMaterialAndComponentsText = Key.SourceMaterialSlot.ToString() + " [" + ComponentNames + "]";

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
				.Text(FText::FromString(SourceMaterialAndComponentsText))
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
								if (Key.Material == StaticMeshComponent->GetMaterial(MaterialIndex))
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
				FMaterialReplacementData MaterialReplacementData;
				MaterialReplacementData.SourceMaterialSlotName = Replacement.Value->SourceMaterialSlot;
				MaterialReplacementData.ReplacementMaterial = Replacement.Value->ReplacementMaterial;
				ReplacementDialogOptions->TargetReplacementAsset->Replacements.Add(MaterialReplacementData);
			}
		}
	}

	TArray<UVitruvioComponent*> ApplyToComponents;
	ApplyToComponents.Add(VitruvioComponent);

	if (ApplyToAllVitruvioActorsCheckBox->IsChecked())
	{
		TArray<AActor*> Actors;
		if (const UWorld* World = GEngine->GetWorldFromContextObject(VitruvioComponent, EGetWorldErrorMode::LogAndReturnNull))
		{
			for (TActorIterator<AActor> It(World, AActor::StaticClass()); It; ++It)
			{
				if (UVitruvioComponent* Component = It->FindComponentByClass<UVitruvioComponent>())
				{
					ApplyToComponents.Add(Component);
				}
			}
		}
	}

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

void FMaterialReplacementDialog::OpenDialog(UVitruvioComponent* VitruvioComponent)
{
	FReplacementDialog::OpenDialog<SMaterialReplacementDialogWidget>(VitruvioComponent);
}
