// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once


#if WITH_EDITOR
#include "IDetailCustomization.h"
#include "DetailCustomizations.h"
#include "IPropertyTypeCustomization.h"
//#include "Slate.h"
//#include "Styling/SlateStyle.h"
//#include "InputCoreTypes.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Colors/SColorPicker.h"
#include "PRTUtilities.h"

struct FCEAttribute;
class APRTActor;

//#define WRITE_TO_DISK_BUTTON

class FPrtDetail : public IDetailCustomization
{
public:
	FPrtDetail();

	UFUNCTION(BlueprintCallable, Category = "RPK Attributes")
	FReply Refresh();

	virtual void CustomizeDetails(IDetailLayoutBuilder& InDetailBuilder) override;
	static TSharedRef<IDetailCustomization>MakeInstance();
	
	IDetailLayoutBuilder *DetailBuilderPtr=nullptr;
	TWeakObjectPtr<class APRTActor> PRTActor;

	static TSharedPtr<ISlateStyle> Style;
private:
	TSharedPtr<SComboBox<TSharedPtr<FString>>> RPKFileSelector;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> OBJFileSelector;

	TArray<TSharedPtr<FString>> RPKFileOptions;
	TSharedPtr<FString> RPKFileSelected;
	TArray<TSharedPtr<FString>> OBJFileOptions;
	TSharedPtr<FString> OBJFileSelected;

	void AddGroupRow(FString Name) const;
	TSharedRef<SWidget> GetComboOption(TSharedPtr<FString> InOption);
	void AddRPKFileSelector();
	void AddOBJFileSelector();
	void AddGenerateButton();
	void AddWriteToDiskButton();

	//call backs for widgets:
	void HandleRPKFileChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type);
	void HandleOBJFileChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type);
	FReply HandleGenerateClicked() const;
	FReply HandleWriteToDiskClicked() const;

	//styles
	//builds the Style for use while building the layout. The Style contains all styles for every element in the UI.
	TSharedRef< ISlateStyle > BuildStyle(); 
	//Handle Registering the style to the static style pointer and registering the style for use by Slate.
	void SetStyle(const TSharedRef< ISlateStyle >& NewStyle );

	FPRTUtilities PRTUtil;
};
#endif