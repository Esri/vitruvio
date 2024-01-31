// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#include "ReplacementDialog.h"

#include "DetailLayoutBuilder.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "ISinglePropertyView.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"

TArray<UVitruvioComponent*> SReplacementDialogWidget::GetVitruvioActorsToApplyReplacements(bool bIncludeAll) const
{
	TArray<UVitruvioComponent*> ApplyToComponents;
	ApplyToComponents.Add(VitruvioComponent);

	if (bIncludeAll)
	{
		TArray<AActor*> Actors;
		if (const UWorld* World = GEngine->GetWorldFromContextObject(VitruvioComponent, EGetWorldErrorMode::LogAndReturnNull))
		{
			for (TActorIterator<AActor> It(World, AActor::StaticClass()); It; ++It)
			{
				if (UVitruvioComponent* Component = It->FindComponentByClass<UVitruvioComponent>())
				{
					if (Component->GetRpk() == VitruvioComponent->GetRpk())
					{
						ApplyToComponents.Add(Component);
					}
				}
			}
		}
	}
	return ApplyToComponents;
}

void SReplacementDialogWidget::AddCommonDialogOptions(const TSharedPtr<SVerticalBox>& Content)
{
	// clang-format off
	Content->AddSlot()
	.Padding(4, 12, 4, 4)
	.AutoHeight()
	[
		SAssignNew(OverrideExistingReplacements, SCheckBox)
		.IsChecked(true)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f))
			.Text(FText::FromString("Override Existing Replacements"))
		]
	];
	// clang-format on
}

void SReplacementDialogWidget::Construct(const FArguments& InArgs)
{
	WeakParentWindow = InArgs._ParentWindow;
	VitruvioComponent = InArgs._VitruvioComponent;

	WeakParentWindow.Pin()->GetOnWindowClosedEvent().AddLambda([this, InArgs](const TSharedRef<SWindow>&) {
		OnWindowClosed();

		// We either want to regenerate if replacements have been applied or if we regenerated the model without replacements when opening the dialog
		if (bReplacementsApplied || InArgs._GeneratedWithoutReplacements)
		{
			VitruvioComponent->Generate();
		}
	});

	const TSharedPtr<ISinglePropertyView> TargetReplacementWidget = CreateTargetReplacementWidget();
	TargetReplacementWidget->GetPropertyHandle()->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([this]() {
		UpdateApplyButtonEnablement();

		UpdateReplacementTable();
	}));

	const FText HeaderText = CreateHeaderText();

	TSharedPtr<SVerticalBox> ContentVerticalBox;

	// clang-format off
	ChildSlot
	[
		SAssignNew(ContentVerticalBox, SVerticalBox)
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.Padding(4.0f)
		.AutoHeight()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text(HeaderText)
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
				OnCreateNewAsset();
					
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(FText::FromString("Create New Asset"))
			]
		]
	];

	AddCommonDialogOptions(ContentVerticalBox);
	AddDialogOptions(ContentVerticalBox);
	
	ContentVerticalBox->AddSlot()
	.Padding(4.0f)
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SAssignNew(ReplacementsBox, SScrollBox)
	];
	
	ContentVerticalBox->AddSlot()
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
				.OnClicked(this, &SReplacementDialogWidget::OnReplacementConfirmed)
			]

			+ SUniformGridPanel::Slot(1, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString("Cancel"))
				.OnClicked(this, &SReplacementDialogWidget::OnReplacementCanceled)
			]
		]
	];
	// clang-format on

	UpdateReplacementTable();
}
