// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once


#if WITH_EDITOR
#include "IDetailCustomization.h"
#include "DetailCustomizations.h"
//#include "IPropertyTypeCustomization.h"
//#include "Slate.h"
//#include "Styling/SlateStyle.h"
//#include "InputCoreTypes.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Colors/SColorPicker.h"

struct FCEAttribute;
class APRTActor;

class FRPKWidget
{
	public:
		void SetAttribute(int32 InGroupIndex, int32 InAttrIndex, FCEAttribute& InAttr, APRTActor* InPRTActor,
		                  IDetailLayoutBuilder* InDetailBuilderPtr, FString InGroup);
		void Create();
		void Update();
		void Destroy();
	private:
		FCEAttribute *attr = nullptr;
		//the indexes for data syncing
		int32 GroupIndex = 0; 
		int32 AttrIndex = 0;
		FString Group;
		APRTActor* PrtActor;
		IDetailLayoutBuilder *DetailBuilderPtr;
		bool bIsDirty = true;

		TSharedPtr<SEditableTextBox> WString; //slate string value
		TSharedPtr<SSlider> WSlider; //slate slider value.
		TSharedPtr<SCheckBox> WBool; //slate Checkbox.
		TSharedPtr<SCheckBox> WVR; //slate VR Checkbox.
		TSharedPtr<SColorPicker> WColor; //color pickers
		TSharedPtr<SComboBox<TSharedPtr<FString>>> WCombo; //combo boxes.
		TArray<TSharedPtr<FString>> WComboOptions; //combo box options
		TSharedPtr<FString> WComboSelected;
		TSharedPtr<SButton> WFilePicker; //File Picker Buttons
		FText GetComboOptionSelected() const;

		//The Different Widget Builders:
		void AddTextWidget();
		void AddSliderWidget();
		void AddColorPickerWidget();
		void AddComboBox();
		void AddCheckBox();
		void AddFilePicker();
		TSharedRef<SWidget> GetComboOption(TSharedPtr<FString> InOption);
		TSharedRef<SCheckBox> VRCheckBox();

		//Callbacks for widgets:
		void HandleTextChanged(const FText& NewText,ETextCommit::Type Type) const;
		void HandleSliderChanged(const float NewFloat) const;
		void HandleCheckboxChanged(const ECheckBoxState NewState ) const;
		void HandleVRCheckboxChanged(const ECheckBoxState NewState ) const;
		void HandleComboBoxChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type);
		FReply HandleFilePickerClicked();
		void HandleColorPicker(FLinearColor NewColor) const;
};
#endif