// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#include "InstanceReplacementDialog.h"

#include "AssetToolsModule.h"
#include "DetailLayoutBuilder.h"
#include "EngineUtils.h"
#include "Factories/DataAssetFactory.h"
#include "Framework/Docking/TabManager.h"
#include "ISinglePropertyView.h"
#include "ReplacementDataAssetFactory.h"
#include "VitruvioComponent.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"

class SInstanceReplacementPackagePicker : public SCompoundWidget, public FGCObject
{
	TWeakPtr<SWindow> WeakParentWindow;
	UVitruvioComponent* VitruvioComponent = nullptr;
	UInstanceReplacementDialogOptions* ReplacementDialogOptions = nullptr;

	TSharedPtr<SScrollBox> ReplacementsBox;
	TArray<TSharedPtr<SCheckBox>> IsolateCheckboxes;
	TSharedPtr<SCheckBox> ApplyToAllVitruvioActorsCheckBox;
	TSharedPtr<SButton> ApplyButton;

	bool bPressedOk = false;

public:
	SLATE_BEGIN_ARGS(SInstanceReplacementPackagePicker) {}
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

void SInstanceReplacementPackagePicker::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ReplacementDialogOptions);
}

void SInstanceReplacementPackagePicker::UpdateReplacementTable()
{
	ReplacementsBox->ClearChildren();
	IsolateCheckboxes.Empty();

	ReplacementDialogOptions->InstanceReplacements.Empty();

	TMap<FString, UStaticMesh*> CurrentReplacements;
	if (ReplacementDialogOptions->TargetReplacementAsset)
	{
		for (const FInstanceReplacementData& ReplacementData : ReplacementDialogOptions->TargetReplacementAsset->Replacements)
		{
			CurrentReplacements.Add(ReplacementData.SourceMeshIdentifier, ReplacementData.ReplacementMesh);
		}
	}

	for (UGeneratedModelHISMComponent* HISMComponent : VitruvioComponent->GetGeneratedModelHISMComponents())
	{
		UInstanceReplacement* InstanceReplacement = NewObject<UInstanceReplacement>();
		InstanceReplacement->SourceMeshIdentifier = HISMComponent->GetMeshIdentifier();

		UStaticMesh** ReplacementMesh = CurrentReplacements.Find(HISMComponent->GetMeshIdentifier());
		if (ReplacementMesh)
		{
			InstanceReplacement->ReplacementMesh = *ReplacementMesh;
		}
		ReplacementDialogOptions->InstanceReplacements.Add(FInstanceKey{HISMComponent->GetMeshIdentifier(), HISMComponent}, InstanceReplacement);
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

		FString MeshIdentifier = Key.MeshComponent->GetStaticMesh()->GetName();
		if (!Key.SourceMeshIdentifier.IsEmpty())
		{
			MeshIdentifier += " [" + Key.SourceMeshIdentifier + "]";
		}

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
				SAssignNew(IsolateCheckbox, SCheckBox)
				.OnCheckStateChanged_Lambda(
					[ResetMaterialPreview, Key, IgnoreIndex = IsolateCheckboxes.Num(), Replacement, this](ECheckBoxState CheckBoxState)
					{
						ResetMaterialPreview(IsolateCheckboxes, IgnoreIndex);

						VitruvioComponent->GetGeneratedModelComponent()->SetVisibility(CheckBoxState != ECheckBoxState::Checked, false);
						
						for (const auto& [OtherKey, OtherReplacement] : ReplacementDialogOptions->InstanceReplacements)
						{
							const bool bVisible = (CheckBoxState == ECheckBoxState::Checked && Replacement == OtherReplacement) ||
													CheckBoxState == ECheckBoxState::Unchecked;
							OtherKey.MeshComponent->SetVisibility(bVisible, false);
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
			Replacement, GET_MEMBER_NAME_CHECKED(UInstanceReplacement, ReplacementMesh), SinglePropertyArgs);
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

void SInstanceReplacementPackagePicker::Construct(const FArguments& InArgs)
{
	WeakParentWindow = InArgs._ParentWindow;
	VitruvioComponent = InArgs._VitruvioComponent;

	ReplacementDialogOptions = NewObject<UInstanceReplacementDialogOptions>();
	ReplacementDialogOptions->TargetReplacementAsset = VitruvioComponent->InstanceReplacement;

	WeakParentWindow.Pin()->GetOnWindowClosedEvent().AddLambda([this](const TSharedRef<SWindow>&) {
		for (const auto& [InstanceKey, Replacement] : ReplacementDialogOptions->InstanceReplacements)
		{
			if (VitruvioComponent->GetGeneratedModelComponent())
			{
				VitruvioComponent->GetGeneratedModelComponent()->SetVisibility(true, false);
			}

			InstanceKey.MeshComponent->SetVisibility(true, false);
		}
	});

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FSinglePropertyParams SinglePropertyArgs;
	SinglePropertyArgs.NamePlacement = EPropertyNamePlacement::Hidden;

	const FString ApplyToAllCheckBoxText = TEXT("Apply to all '") + VitruvioComponent->GetRpk()->GetName() + TEXT("' VitruvioActors");
	const TSharedPtr<ISinglePropertyView> TargetReplacementWidget = PropertyEditorModule.CreateSingleProperty(
		ReplacementDialogOptions, GET_MEMBER_NAME_CHECKED(UInstanceReplacementDialogOptions, TargetReplacementAsset), SinglePropertyArgs);

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
			.Text(FText::FromString(TEXT("Choose Instance replacements and the DataTable where they will be added.")))
		]

		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SBox)
			.MinDesiredWidth(250)
			[
				TargetReplacementWidget.ToSharedRef()
			]
		]
		
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SButton)
			.OnClicked_Lambda([this]()
			{
				if (const auto Window = WeakParentWindow.Pin())
				{
					const FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
					UReplacementDataAssetFactory* DataAssetFactory = NewObject<UReplacementDataAssetFactory>();
					Window->HideWindow();
					DataAssetFactory->DataAssetClass = UInstanceReplacementAsset::StaticClass();
					if (UInstanceReplacementAsset* NewReplacementAsset = Cast<UInstanceReplacementAsset>(AssetToolsModule.Get().CreateAssetWithDialog(UInstanceReplacementAsset::StaticClass(), DataAssetFactory)))
					{
						ReplacementDialogOptions->TargetReplacementAsset = NewReplacementAsset;
					}
					
					Window->ShowWindow();

					ApplyButton->SetEnabled(ReplacementDialogOptions->TargetReplacementAsset != nullptr);
				}
				
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(FText::FromString("Create New Asset"))
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
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			[
				SNew(SUniformGridPanel).SlotPadding(2)
				+ SUniformGridPanel::Slot(0, 0)
				[
					SAssignNew(ApplyButton, SButton)
					.HAlign(HAlign_Center)
					.Text(FText::FromString("Apply"))
					.OnClicked(this, &SInstanceReplacementPackagePicker::OnReplacementConfirmed)
				]

				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(FText::FromString("Cancel"))
					.OnClicked(this, &SInstanceReplacementPackagePicker::OnReplacementCanceled)
				]
			]
		]
	];
	// clang-format on

	ApplyButton->SetEnabled(ReplacementDialogOptions->TargetReplacementAsset != nullptr);
	UpdateReplacementTable();
}

FReply SInstanceReplacementPackagePicker::OnReplacementConfirmed()
{
	bPressedOk = true;

	if (ReplacementDialogOptions->TargetReplacementAsset)
	{
		for (const auto& Replacement : ReplacementDialogOptions->InstanceReplacements)
		{
			if (Replacement.Value->ReplacementMesh)
			{
				FInstanceReplacementData InstanceReplacementData;
				InstanceReplacementData.SourceMeshIdentifier = Replacement.Value->SourceMeshIdentifier;
				InstanceReplacementData.ReplacementMesh = Replacement.Value->ReplacementMesh;
				ReplacementDialogOptions->TargetReplacementAsset->Replacements.Add(InstanceReplacementData);
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
		Component->InstanceReplacement = ReplacementDialogOptions->TargetReplacementAsset;
		Component->Generate();
	}

	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply SInstanceReplacementPackagePicker::OnReplacementCanceled()
{
	if (WeakParentWindow.IsValid())
	{
		WeakParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

void FInstanceReplacementDialog::OpenDialog(UVitruvioComponent* VitruvioComponent)
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

	TSharedRef<SInstanceReplacementPackagePicker> ReplacementPicker =
		SNew(SInstanceReplacementPackagePicker).VitruvioComponent(VitruvioComponent).ParentWindow(PickerWindow);
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
