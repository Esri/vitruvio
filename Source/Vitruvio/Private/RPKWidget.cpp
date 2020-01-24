

#if WITH_EDITOR
#include "RPKWidget.h"
#include "PRTDetail.h"
#include "PRTActor.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets\Input\SEditableTextBox.h"

#define LOCTEXT_NAMESPACE "PrtDetail"

void FRPKWidget::SetAttribute(int32 InGroupIndex, int32 InAttrIndex, FCEAttribute& inAttr, APRTActor* InPRTActor,
                              IDetailLayoutBuilder* InDetailBuilderPtr, FString InGroup)
{
	Attribute = &inAttr;
	GroupIndex = InGroupIndex;
	AttrIndex = InAttrIndex;
	PrtActor = InPRTActor;
	DetailBuilderPtr = InDetailBuilderPtr;
	Group = InGroup;
}

void FRPKWidget::Create()
{
	Destroy();

	if (!Attribute->bHidden)
	{
		switch (Attribute->Widget)
		{
		case ERPKWidgetTypes::GENERAL_TEXT: AddTextWidget();
			break;
		case ERPKWidgetTypes::SLIDER: AddSliderWidget();
			break;
		case ERPKWidgetTypes::COLOR: AddColorPickerWidget();
			break;
		case ERPKWidgetTypes::COMBO: AddComboBox();
			break;
		case ERPKWidgetTypes::FILE: break;
			AddFilePicker();
			break;
			//case RPKWidgetTypes::FILE: break; AddTextWidget(); break;
		case ERPKWidgetTypes::DIRECTORY: break;
			AddTextWidget();
			break;
		case ERPKWidgetTypes::CHECKBOX: AddCheckBox();
			break;
		case ERPKWidgetTypes::NUMBER_TEXT: AddTextWidget();
			break;
		}
	}
}

void FRPKWidget::Update()
{
	TAttribute<ECheckBoxState> IsChecked;

	if (Attribute != nullptr)
	{
		if (!Attribute->bHidden)
		{
			switch (Attribute->Widget)
			{
			case ERPKWidgetTypes::GENERAL_TEXT:
				if (WString.IsValid())
				{
					WString->SetText(FText::FromString(Attribute->sValue));
				}
				break;
			case ERPKWidgetTypes::SLIDER:
				if (WString.IsValid())
				{
					WString->SetText(FText::FromString(FString::SanitizeFloat(Attribute->fValue)));
				}
				if (WSlider.IsValid())
				{
					float value = Attribute->fValue;
					float range = Attribute->Range[1] - Attribute->Range[0];

					//constrain by step, to do this we need to emulate round to nearest step.
					value += Attribute->Step / 2.0; //add half a step, so a truncate will be between -halfstep/+halfstep.
					int32 cValue = value / Attribute->Step; //use an int to truncate it for us.
					value = cValue * Attribute->Step;

					//set slider from value
					float sliderValue = value;
					sliderValue -= Attribute->Range[0];
					sliderValue = sliderValue / range;

					//constrain the resulting slider value
					if (sliderValue < 0.0) sliderValue = 0.0;
					if (sliderValue > 1.0) sliderValue = 1.0;
					//constrain float value within min and max.
					if (value < Attribute->Range[0]) value = Attribute->Range[0];
					if (value > Attribute->Range[1]) value = Attribute->Range[1];

					//set the value to the slider and float
					WSlider->SetValue(sliderValue);
					Attribute->fValue = value;
				}
				break;
			case ERPKWidgetTypes::COLOR:
				//DetailBuilderPtr->ForceRefreshDetails();
				break;
			case ERPKWidgetTypes::COMBO:
				//DetailBuilderPtr->ForceRefreshDetails();
				//can't refresh here.
				break;
			case ERPKWidgetTypes::FILE:
				if (WString.IsValid())
				{
					WString->SetText(FText::FromString(Attribute->sValue));
				}
				break;
			case ERPKWidgetTypes::DIRECTORY:
				if (WString.IsValid())
				{
					WString->SetText(FText::FromString(Attribute->sValue));
				}
				break;
			case ERPKWidgetTypes::CHECKBOX:
				if (WBool.IsValid())
				{
					if (Attribute->bValue)
					{
						IsChecked = ECheckBoxState::Checked;
					}
					else
					{
						IsChecked = ECheckBoxState::Unchecked;
					}
					WBool->SetIsChecked(IsChecked);
				}
				break;
			case ERPKWidgetTypes::NUMBER_TEXT:
				if (WString.IsValid())
				{
					WString->SetText(FText::FromString(FString::SanitizeFloat(Attribute->fValue)));
				}
				break;
			}
		}
	}
}

/****************************************************************************************************************
*
*		Widget Builders:
*
****************************************************************************************************************/
TSharedRef<SCheckBox> FRPKWidget::VRCheckBox()
{
	TAttribute<ECheckBoxState> vrState;
	if (Attribute->showInVR)
	{
		vrState.Set(ECheckBoxState::Checked);
	}
	else
	{
		vrState.Set(ECheckBoxState::Unchecked);
	}

	TSharedRef<SCheckBox> VRcheckBox = SNew(SCheckBox)
	.Style(*FPRTDetail::Style.Get(), "VRCheckBox")
	.IsChecked(vrState)
	.OnCheckStateChanged_Raw(this, &FRPKWidget::HandleVRCheckboxChanged);
	WVR = VRcheckBox;

	return VRcheckBox;
}

void FRPKWidget::AddTextWidget()
{
	if (DetailBuilderPtr != nullptr && Attribute != nullptr)
	{
		//For the Slate Attributes Panel, the design is the same for both general numbers and strings.
		FString strValue;
		if (Attribute->Type == 1)
		{
			strValue = FString::SanitizeFloat(Attribute->fValue);
		}
		else
		{
			strValue = Attribute->sValue;
		}

		TSharedRef<SEditableTextBox> value = SNew(SEditableTextBox)
			.Text(FText::FromString(strValue))
			.OnTextCommitted_Raw(this, &FRPKWidget::HandleTextChanged);
		WString = value;

		TSharedRef<SHorizontalBox> ValueContent = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  [
				  value
			  ]
			  .Padding(FMargin(1.0f))
			  .MaxWidth(200.0f)
			  .AutoWidth()
			  .Padding(FMargin(1.0f))
			  .HAlign(HAlign_Left);
		if (Attribute->bIsPercentage)
		{
			ValueContent->AddSlot()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString(L"%")))
				]
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.AutoWidth();
		}
		ValueContent->AddSlot()
		            .VAlign(VAlign_Fill)
		            .HAlign(HAlign_Fill)
		            .FillWidth(100.0f);
		ValueContent->AddSlot()
			[
				VRCheckBox()
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(FMargin(1.0f))
			.AutoWidth();
		ValueContent->AddSlot()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString(L"VR")))
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(FMargin(1.0f))
			.AutoWidth();

		//build the final row
		DetailBuilderPtr->EditCategory(*Group, FText::GetEmpty(), ECategoryPriority::Important)
		                .AddCustomRow(FText::FromString(Attribute->DisplayName))
		                .NameContent()
			[
				SNew(STextBlock).Text(FText::FromString(Attribute->DisplayName))
			]
			.ValueContent()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				ValueContent
			];
	}
}

void FRPKWidget::AddSliderWidget()
{
	//Sliders are always from 0.0 to 1.0
	float sliderValue = 0.0;
	if (Attribute->Range.Num() > 0)
	{
		sliderValue = Attribute->fValue;
		float range = Attribute->Range[1] - Attribute->Range[0];
		sliderValue -= Attribute->Range[0];
		sliderValue = sliderValue / range;
	}

	TSharedRef<SEditableTextBox> value = SNew(SEditableTextBox)
		.Text(FText::FromString(FString::SanitizeFloat(Attribute->fValue)))
		.OnTextCommitted_Raw(this, &FRPKWidget::HandleTextChanged);
	WString = value;
	if (Attribute->SliderStep < .01) Attribute->SliderStep = .01;
	TSharedRef<SSlider> Slider = SNew(SSlider)
			.Value(sliderValue)
			.StepSize(Attribute->SliderStep)
			.MouseUsesStep(true)
			.OnValueChanged_Raw(this, &FRPKWidget::HandleSliderChanged);
	WSlider = Slider;

	TSharedRef<SHorizontalBox> ValueContent = SNew(SHorizontalBox);

	ValueContent->AddSlot()
		[
			value
		]
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Left)
		.Padding(FMargin(1.0f))
		.MaxWidth(40.0f)
		.AutoWidth();
	if (Attribute->bIsPercentage)
	{
		ValueContent->AddSlot()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString(L"%")))
			]
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.AutoWidth();
	}
	ValueContent->AddSlot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::SanitizeFloat(Attribute->Range[0])))
		]
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Center)
		.Padding(FMargin(1.0f))
		.AutoWidth();
	ValueContent->AddSlot()
		[
			Slider
		]
		.VAlign(VAlign_Fill)
		.Padding(FMargin(1.0f))
		.HAlign(HAlign_Fill)
		.FillWidth(100.0f);
	ValueContent->AddSlot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::SanitizeFloat(Attribute->Range[1])))
		]
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Center)
		.Padding(FMargin(1.0f))
		.AutoWidth();
	ValueContent->AddSlot()
		[
			VRCheckBox()
		]
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.Padding(FMargin(1.0f))
		.AutoWidth();
	ValueContent->AddSlot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString(L"VR")))
		]
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.Padding(FMargin(1.0f))
		.AutoWidth();

	//build the final row
	DetailBuilderPtr->EditCategory(*Group, FText::GetEmpty(), ECategoryPriority::Important)
	                .AddCustomRow(FText::FromString(Attribute->DisplayName))
	                .NameContent()
		[
			SNew(STextBlock).Text(FText::FromString(Attribute->DisplayName))
		]
		.ValueContent()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			ValueContent
		];
}

void FRPKWidget::AddColorPickerWidget()
{
	TSharedRef<SColorPicker> value = SNew(SColorPicker)
		.OnColorCommitted_Raw(this, &FRPKWidget::HandleColorPicker)
		.UseAlpha(false)
		.DisplayInlineVersion(false)
		.TargetColorAttribute(Attribute->Color);
	WColor = value;

	TSharedRef<SHorizontalBox> ValueContent = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			value
		]
		+ SHorizontalBox::Slot()
		  [
			  VRCheckBox()
		  ]
		  .VAlign(VAlign_Center)
		  .HAlign(HAlign_Center)
		  .Padding(FMargin(1.0f))
		  .AutoWidth()
		+ SHorizontalBox::Slot()
		  [
			  SNew(STextBlock)
			  .Text(FText::FromString(FString(L"VR")))
		  ]
		  .VAlign(VAlign_Center)
		  .HAlign(HAlign_Center)
		  .Padding(FMargin(1.0f))
		  .AutoWidth();

	//build the final row
	DetailBuilderPtr->EditCategory(*Group, FText::GetEmpty(), ECategoryPriority::Important)
	                .AddCustomRow(FText::FromString(Attribute->DisplayName))
	                .NameContent()
		[
			SNew(STextBlock).Text(FText::FromString(Attribute->DisplayName))
		]
		.ValueContent()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			ValueContent
		];
}

TSharedRef<SWidget> FRPKWidget::GetComboOption(TSharedPtr<FString> inOption)
{
	return SNew(STextBlock).Text(FText::FromString(*inOption));
}

FText FRPKWidget::GetComboOptionSelected() const
{
	if (WCombo.IsValid())
	{
		TSharedPtr<FString> current = WCombo->GetSelectedItem();
		return FText::FromString(*current);
	}
	return FText::FromString(L"Error");
}

void FRPKWidget::AddComboBox()
{
	if (WComboOptions.Num() == 0)
	{
		for (int32 i = 0; i < Attribute->SelectValues.Num(); i++)
		{
			TSharedPtr<FString> Temp = MakeShareable(new FString(Attribute->SelectValues[i]));
			WComboOptions.Add(Temp);
			if (Attribute->sValue.Compare(Attribute->SelectValues[i]) == 0)
			{
				WComboSelected = Temp;
			}
		}
	}

	const TSharedRef<SComboBox<TSharedPtr<FString>>> Value = SNew(SComboBox<TSharedPtr<FString>>)
		.OptionsSource(&WComboOptions)
		.OnGenerateWidget_Raw(this, &FRPKWidget::GetComboOption)
		.OnSelectionChanged_Raw(this, &FRPKWidget::HandleComboBoxChanged)
		.InitiallySelectedItem(WComboSelected)[
		SNew(STextBlock)
		.Text_Raw(this, &FRPKWidget::GetComboOptionSelected)
	];
	WCombo = Value;

	TSharedRef<SHorizontalBox> ValueContent = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		  [
			  Value
		  ]
		  .Padding(FMargin(1.0f))
		  .MaxWidth(200.0f)
		  .AutoWidth()
		  .Padding(FMargin(1.0f))
		  .HAlign(HAlign_Left)
		+ SHorizontalBox::Slot()
		  .VAlign(VAlign_Fill)
		  .HAlign(HAlign_Fill)
		  .FillWidth(100.0f)
		+ SHorizontalBox::Slot()
		  [
			  VRCheckBox()
		  ]
		  .VAlign(VAlign_Center)
		  .HAlign(HAlign_Center)
		  .Padding(FMargin(1.0f))
		  .AutoWidth()
		+ SHorizontalBox::Slot()
		  [
			  SNew(STextBlock)
			  .Text(FText::FromString(FString(L"VR")))
		  ]
		  .VAlign(VAlign_Center)
		  .HAlign(HAlign_Center)
		  .Padding(FMargin(1.0f))
		  .AutoWidth();

	//build the final row
	DetailBuilderPtr->EditCategory(*Group, FText::GetEmpty(), ECategoryPriority::Important)
	                .AddCustomRow(FText::FromString(Attribute->DisplayName))
	                .NameContent()
		[
			SNew(STextBlock).Text(FText::FromString(Attribute->DisplayName))
		]
		.ValueContent()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			ValueContent
		];
}

void FRPKWidget::AddCheckBox()
{
	//makes the detail panel content
	TAttribute<ECheckBoxState> state;
	if (Attribute->bValue)
	{
		state.Set(ECheckBoxState::Checked);
	}
	else
	{
		state.Set(ECheckBoxState::Unchecked);
	}

	//build bool checkbox
	TSharedRef<SCheckBox> Value = SNew(SCheckBox)
		.IsChecked(state)
		.OnCheckStateChanged_Raw(this, &FRPKWidget::HandleCheckboxChanged);
	WBool = Value;

	TSharedRef<SHorizontalBox> ValueContent = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			Value
		]
		.VAlign(VAlign_Fill)
		+ SHorizontalBox::Slot()
		  [
			  VRCheckBox()
		  ]
		  .VAlign(VAlign_Center)
		  .HAlign(HAlign_Center)
		  .Padding(FMargin(1.0f))
		  .AutoWidth()
		+ SHorizontalBox::Slot()
		  [
			  SNew(STextBlock)
			  .Text(FText::FromString(FString(L"VR")))
		  ]
		  .VAlign(VAlign_Center)
		  .HAlign(HAlign_Center)
		  .Padding(FMargin(1.0f))
		  .AutoWidth();

	//build the final row
	DetailBuilderPtr->EditCategory(*Group, FText::GetEmpty(), ECategoryPriority::Important)
	                .AddCustomRow(FText::FromString(Attribute->DisplayName))
	                .NameContent()
		[
			SNew(STextBlock).Text(FText::FromString(Attribute->DisplayName))
		]
		.ValueContent()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			ValueContent
		];
}

void FRPKWidget::AddFilePicker()
{
	//makes the detail panel content
	TAttribute<ECheckBoxState> State;
	if (Attribute->bValue)
	{
		State.Set(ECheckBoxState::Checked);
	}
	else
	{
		State.Set(ECheckBoxState::Unchecked);
	}

	TSharedRef<SButton> Value = SNew(SButton).OnClicked_Raw(this, &FRPKWidget::HandleFilePickerClicked);

	WFilePicker = Value;

	TSharedRef<SHorizontalBox> ValueContent = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			Value
		]
		.VAlign(VAlign_Fill)
		+ SHorizontalBox::Slot()
		  [
			  VRCheckBox()
		  ]
		  .VAlign(VAlign_Center)
		  .HAlign(HAlign_Center)
		  .Padding(FMargin(1.0f))
		  .AutoWidth()
		+ SHorizontalBox::Slot()
		  [
			  SNew(STextBlock)
			  .Text(FText::FromString(FString(L"VR")))
		  ]
		  .VAlign(VAlign_Center)
		  .HAlign(HAlign_Center)
		  .Padding(FMargin(1.0f))
		  .AutoWidth();

	//build the final row
	DetailBuilderPtr->EditCategory(*Group, FText::GetEmpty(), ECategoryPriority::Important)
	                .AddCustomRow(FText::FromString(Attribute->DisplayName))
	                .NameContent()
		[
			SNew(STextBlock).Text(FText::FromString(Attribute->DisplayName))
		]
		.ValueContent()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			ValueContent
		];
}

/****************************************************************************************************************
*
*		Widget Callbacks:
*
****************************************************************************************************************/
void FRPKWidget::HandleSliderChanged(const float NewFloat) const
{
	if (Attribute != nullptr)
	{
		if (Attribute->Type == 1 && Attribute->Range.Num() > 0)
		{
			const double Value = NewFloat * (Attribute->Range[1] - Attribute->Range[0]) + Attribute->Range[0];
			PrtActor->SyncAttributeFloat(GroupIndex, AttrIndex, Value);
		}
	}
}

void FRPKWidget::HandleTextChanged(const FText& NewText, ETextCommit::Type Type) const
{
	if (Attribute != nullptr)
	{
		if (Attribute->Type == 1)
		{
			const float Value = FCString::Atof(*NewText.ToString());
			PrtActor->SyncAttributeFloat(GroupIndex, AttrIndex, Value);
		}
		if (Attribute->Type == 2)
		{
			PrtActor->SyncAttributeString(GroupIndex, AttrIndex, NewText.ToString());
		}
	}
}

void FRPKWidget::HandleCheckboxChanged(const ECheckBoxState NewState) const
{
	if (Attribute != nullptr)
	{
		switch (Attribute->Type)
		{
		case 0: //bool
			if (NewState == ECheckBoxState::Checked)
			{
				PrtActor->SyncAttributeBool(GroupIndex, AttrIndex, true);
			}
			else
			{
				PrtActor->SyncAttributeBool(GroupIndex, AttrIndex, false);
			}
			break;
		default: ;
		}
	}
}

void FRPKWidget::HandleVRCheckboxChanged(const ECheckBoxState NewState) const
{
	if (Attribute != nullptr)
	{
		if (NewState == ECheckBoxState::Checked)
		{
			Attribute->showInVR = true;
		}
		else
		{
			Attribute->showInVR = false;
		}
		PrtActor->CopyViewAttributesIntoDataStore();
	}
}

FReply FRPKWidget::HandleFilePickerClicked()
{
	//I need to figure out a way to determine who called this.
	//done, now do it.

	return FReply::Handled();
}

void FRPKWidget::HandleColorPicker(const FLinearColor NewColor) const
{
	if (Attribute != nullptr)
	{
		if (Attribute->Widget == ERPKWidgetTypes::COLOR)
		{
			PrtActor->SyncAttributeColor(GroupIndex, AttrIndex, NewColor);
		}
	}
}

void FRPKWidget::HandleComboBoxChanged(const TSharedPtr<FString> NewValue, ESelectInfo::Type)
{
	WComboSelected = NewValue;
	const FString Value = *NewValue;
	if (Attribute != nullptr)
	{
		if (Attribute->Type == 1)
		{
			PrtActor->SyncAttributeFloat(GroupIndex, AttrIndex, FCString::Atof(*Value));
		}
		if (Attribute->Type == 2)
		{
			PrtActor->SyncAttributeString(GroupIndex, AttrIndex, *Value);
		}
	}

	if (DetailBuilderPtr != nullptr)
	{
		DetailBuilderPtr->ForceRefreshDetails();
	}
}

void FRPKWidget::Destroy()
{
	WString.Reset();
	WSlider.Reset();
	WBool.Reset();
	WVR.Reset();
	WColor.Reset();
	WCombo.Reset();
}
#endif
