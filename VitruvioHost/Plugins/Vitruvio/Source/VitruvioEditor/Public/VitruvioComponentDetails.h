// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "IDetailCustomization.h"
#include "VitruvioComponent.h"

class FVitruvioComponentDetails final : public IDetailCustomization
{
public:
	FVitruvioComponentDetails();
	~FVitruvioComponentDetails();

	static TSharedRef<IDetailCustomization> MakeInstance();

	void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;

	void OnPropertyChanged(UObject* Object, struct FPropertyChangedEvent& Event);

private:
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	TWeakPtr<IDetailLayoutBuilder> CachedDetailBuilder;
	TSharedPtr<SWidget> ColorPickerParentWidget;
};

template <typename T>
class SPropertyComboBox final : public SComboBox<TSharedPtr<T>>
{
	using SelectionChanged = typename SComboBox<TSharedPtr<T>>::FOnSelectionChanged;

public:
	SLATE_BEGIN_ARGS(SPropertyComboBox) {}
	SLATE_ATTRIBUTE(TArray<TSharedPtr<T>>, ComboItemList)
	SLATE_EVENT(SelectionChanged, OnSelectionChanged)
	SLATE_ATTRIBUTE(TSharedPtr<T>, InitialValue)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TArray<TSharedPtr<T>> ComboItemList;

	TSharedRef<SWidget> OnGenerateComboWidget(TSharedPtr<T> InValue) const;
};
