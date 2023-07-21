// Copyright © 2017-2023 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "AssetToolsModule.h"
#include "CoreMinimal.h"
#include "ReplacementDataAssetFactory.h"
#include "VitruvioComponent.h"
#include "Widgets/Layout/SScrollBox.h"

class SReplacementDialogWidget : public SCompoundWidget, public FGCObject
{
protected:
	TWeakPtr<SWindow> WeakParentWindow;
	UVitruvioComponent* VitruvioComponent = nullptr;

	TSharedPtr<SScrollBox> ReplacementsBox;
	TSharedPtr<SButton> ApplyButton;

public:
	SLATE_BEGIN_ARGS(SReplacementDialogWidget) {}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_ARGUMENT(UVitruvioComponent*, VitruvioComponent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	virtual void UpdateReplacementTable() = 0;
	virtual FText CreateHeaderText() = 0;
	virtual TSharedPtr<ISinglePropertyView> CreateTargetReplacementWidget() = 0;
	virtual void OnCreateNewAsset() = 0;
	virtual FReply OnReplacementConfirmed() = 0;
	virtual FReply OnReplacementCanceled() = 0;
	virtual void AddDialogOptions(const TSharedPtr<SVerticalBox>& Content) = 0;
	virtual void OnWindowClosed() = 0;

	template <typename TAsset, typename TDialogOptions>
	void CreateNewAsset(TDialogOptions* DialogOptions)
	{
		if (const auto Window = WeakParentWindow.Pin())
		{
			const FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
			UReplacementDataAssetFactory* DataAssetFactory = NewObject<UReplacementDataAssetFactory>();
			Window->HideWindow();
			DataAssetFactory->DataAssetClass = TAsset::StaticClass();
			if (TAsset* NewReplacementAsset = Cast<TAsset>(AssetToolsModule.Get().CreateAssetWithDialog(TAsset::StaticClass(), DataAssetFactory)))
			{
				DialogOptions->TargetReplacementAsset = NewReplacementAsset;
			}

			Window->ShowWindow();

			ApplyButton->SetEnabled(DialogOptions->TargetReplacementAsset != nullptr);
		}
	}
};

class FReplacementDialog
{
public:
	template <typename TDialogType>
	static void OpenDialog(UVitruvioComponent* VitruvioComponent)
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

		TSharedRef<TDialogType> ReplacementPicker = SNew(TDialogType).VitruvioComponent(VitruvioComponent).ParentWindow(PickerWindow);
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
};
