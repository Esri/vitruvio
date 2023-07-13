// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#include "MaterialReplacementDialog.h"

#include "DetailLayoutBuilder.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "ISinglePropertyView.h"
#include "VitruvioComponent.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"

UMaterialReplacementAsset* PreviousMaterialReplacementTarget = nullptr;

class SCMaterialReplacementPackagePicker : public SCompoundWidget, public FGCObject
{
	TWeakPtr<SWindow> WeakParentWindow;
	UVitruvioComponent* VitruvioComponent = nullptr;
	UMaterialReplacementDialogOptions* ReplacementDialogOptions = nullptr;

	TSharedPtr<SScrollBox> ReplacementsBox;
	TArray<TSharedPtr<SCheckBox>> IsolateCheckboxes;
	TSharedPtr<SCheckBox> IncludeInstancesCheckBox;
	TSharedPtr<SCheckBox> ApplyToAllVitruvioActorsCheckBox;
	TSharedPtr<SButton> ApplyButton;

	bool bPressedOk = false;

public:
	SLATE_BEGIN_ARGS(SCMaterialReplacementPackagePicker) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_ARGUMENT(UVitruvioComponent*, VitruvioComponent)
	SLATE_END_ARGS()

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

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

void SCMaterialReplacementPackagePicker::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ReplacementDialogOptions);
}

void SCMaterialReplacementPackagePicker::UpdateReplacementTable()
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
				MaterialReplacement->SourceMaterialSlot = MaterialSlotName;
				MaterialReplacement->Components.Add(StaticMeshComponent);
				ReplacementDialogOptions->MaterialReplacements.Add({ SourceMaterial, MaterialSlotName }, MaterialReplacement);
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

		const TSharedPtr<ISinglePropertyView> SinglePropertyViewWidget =
			PropertyEditorModule.CreateSingleProperty(Replacement, GET_MEMBER_NAME_CHECKED(UMaterialReplacement, ReplacementMaterial), SinglePropertyArgs);
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

void SCMaterialReplacementPackagePicker::Construct(const FArguments& InArgs)
{
	WeakParentWindow = InArgs._ParentWindow;
	VitruvioComponent = InArgs._VitruvioComponent;

	ReplacementDialogOptions = NewObject<UMaterialReplacementDialogOptions>();
	ReplacementDialogOptions->TargetReplacementAsset = PreviousMaterialReplacementTarget;

	WeakParentWindow.Pin()->GetOnWindowClosedEvent().AddLambda([this](const TSharedRef<SWindow>&) {
		for (const auto& [MaterialName, Replacement] : ReplacementDialogOptions->MaterialReplacements)
		{
			for (UStaticMeshComponent* StaticMeshComponent : Replacement->Components)
			{
				StaticMeshComponent->SetMaterialPreview(INDEX_NONE);
				StaticMeshComponent->SelectedEditorMaterial = INDEX_NONE;
			}
		}
	});

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FSinglePropertyParams SinglePropertyArgs;
	SinglePropertyArgs.NamePlacement = EPropertyNamePlacement::Hidden;

	const FString ApplyToAllCheckBoxText = TEXT("Apply to all '") + VitruvioComponent->GetRpk()->GetName() + TEXT("' VitruvioActors");
	const TSharedPtr<ISinglePropertyView> TargetReplacementWidget = PropertyEditorModule.CreateSingleProperty(
		ReplacementDialogOptions, GET_MEMBER_NAME_CHECKED(UMaterialReplacementDialogOptions, TargetReplacementAsset), SinglePropertyArgs);

	TargetReplacementWidget->GetPropertyHandle()->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateLambda([this]() { ApplyButton->SetEnabled(ReplacementDialogOptions->TargetReplacementAsset != nullptr); }));

	// clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.Padding(4.0f)
		.AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text(FText::FromString(TEXT("Choose Material replacements and the DataTable where they will be added.")))
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
		.Padding(4)
		.AutoHeight()
		[
			SNew(SCheckBox)
			.IsChecked(true)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f))
				.Text(FText::FromString("Preview Replacements"))
			]
		]

		+ SVerticalBox::Slot()
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
		.HAlign(HAlign_Fill)
		.Padding(2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			[
				SNew(SBox)
				.MinDesiredWidth(200)
				[
					TargetReplacementWidget.ToSharedRef()
				]
			]

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			[
				SNew(SUniformGridPanel).SlotPadding(2)
				+ SUniformGridPanel::Slot(0, 0)
				[
					SAssignNew(ApplyButton, SButton)
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
		]
	];
	// clang-format on

	ApplyButton->SetEnabled(ReplacementDialogOptions->TargetReplacementAsset != nullptr);
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

FReply SCMaterialReplacementPackagePicker::OnReplacementCanceled()
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
	// clang-format off
	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
		.Title(FText::FromString("Choose Replacement"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(500.f, 400.f))
		.IsTopmostWindow(true)
		.SupportsMaximize(false)
		.SupportsMinimize(false);
	// clang-format on

	TSharedRef<SCMaterialReplacementPackagePicker> ReplacementPicker =
		SNew(SCMaterialReplacementPackagePicker).VitruvioComponent(VitruvioComponent).ParentWindow(PickerWindow);
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
