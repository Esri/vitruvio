// Epic Games: PRTDetail.cpp


#if WITH_EDITOR
#include "PRTDetail.h"
#include "Interfaces/IPluginManager.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "PRTActor.h"
#include <Styling/SlateStyle.h>
#include <Styling/SlateStyleRegistry.h>
#include <Widgets/Input/SButton.h>

#define LOCTEXT_NAMESPACE "PrtDetail"
TSharedPtr<ISlateStyle> FPRTDetail::Style = nullptr;

TSharedRef<IDetailCustomization> FPRTDetail::MakeInstance()
{
	return MakeShareable(new FPRTDetail);
}

FPRTDetail::FPRTDetail()
{
	SetStyle(BuildStyle());
}

FReply FPRTDetail::Refresh()
{
	if (DetailBuilderPtr != nullptr)
	{
		DetailBuilderPtr->ForceRefreshDetails();
	}
	return FReply::Handled();
}

void FPRTDetail::AddGroupRow(const FString Name) const
{
	if (Name.Len() > 0)
	{
		DetailBuilderPtr->EditCategory("RPK Attributes", FText::GetEmpty(), ECategoryPriority::Important)
		                .AddGroup("group", FText::FromString(""), false, false);
		DetailBuilderPtr->EditCategory("RPK Attributes", FText::GetEmpty(), ECategoryPriority::Important)
		                .AddGroup("group", FText::FromString(Name));
	}
}

/****************************************************************************************************************
*
*		Styles Builders:
*
****************************************************************************************************************/
TSharedRef<ISlateStyle> FPRTDetail::BuildStyle()
{
	const TSharedRef<FSlateStyleSet> NewStyle = MakeShareable(new FSlateStyleSet("DetailStyle"));

	//VR CheckBoxes.
	const FVector2D VRIcon(16.0f, 16.0f);

	FString VRCheckedIconPath = PRTUtil.GetPluginBaseDirectory() + "/Resources/VRChecked.png";
	FString VRUnCheckedIconPath = PRTUtil.GetPluginBaseDirectory() + "/Resources/VRUnchecked.png";

	const FCheckBoxStyle VRCheckBoxStyle = FCheckBoxStyle()
	                                       .SetCheckedImage(FSlateImageBrush(VRCheckedIconPath, VRIcon))
	                                       .SetCheckedHoveredImage(
		                                       FSlateImageBrush(VRCheckedIconPath, VRIcon, FLinearColor(.5f, .5f, .5f)))
	                                       .SetCheckedPressedImage(FSlateImageBrush(
		                                       VRCheckedIconPath, VRIcon, FLinearColor(.25f, .25f, .25f)))
	                                       .SetUncheckedImage(FSlateImageBrush(VRUnCheckedIconPath, VRIcon))
	                                       .SetUncheckedHoveredImage(
		                                       FSlateImageBrush(VRUnCheckedIconPath, VRIcon,
		                                                        FLinearColor(.5f, .5f, .5f)))
	                                       .SetUncheckedPressedImage(FSlateImageBrush(
		                                       VRUnCheckedIconPath, VRIcon, FLinearColor(.25f, .25f, .25f)));
	NewStyle->Set("VRCheckBox", VRCheckBoxStyle);

	return NewStyle;
}

void FPRTDetail::SetStyle(const TSharedRef<ISlateStyle>& NewStyle)
{
	if (Style.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*Style.Get());
	}
	Style = NewStyle;
	if (Style.IsValid())
	{
		FSlateStyleRegistry::RegisterSlateStyle(*Style.Get());
	}
}

/****************************************************************************************************************
*
*		Widget Callbacks:
*
****************************************************************************************************************/
FReply FPRTDetail::HandleGenerateClicked() const
{
	APRTActor* PrtActor = PRTActor.Get();

	PrtActor->Generate();

	return FReply::Handled();
}


/****************************************************************************************************************
*
*		Widget Builders:
*
****************************************************************************************************************/
TSharedRef<SWidget> FPRTDetail::GetComboOption(TSharedPtr<FString> inOption)
{
	return SNew(STextBlock).Text(FText::FromString(*inOption));
}

void FPRTDetail::AddRPKFileSelector()
{
	APRTActor* PrtActor = PRTActor.Get();
	const FString Selected = PrtActor->RPKFile;

	if (RPKFileOptions.Num() == 0)
	{
		for (auto i = 0; i < PrtActor->RPKFiles.Num(); i++)
		{
			TSharedPtr<FString> Temp = MakeShareable(new FString(PrtActor->RPKFiles[i].Name));
			RPKFileOptions.Add(Temp);
			if (PrtActor->RPKFiles[i].Name.Compare(Selected) == 0)
			{
				RPKFileSelected = Temp;
			}
		}
		//just in case.
		if (RPKFileOptions.Num() > 0 && !RPKFileSelected.IsValid())
		{
			RPKFileSelected = RPKFileOptions[0];
		}
	}

	if (RPKFileOptions.Num() > 0 && RPKFileSelected.IsValid())
	{
		TSharedRef<SComboBox<TSharedPtr<FString>>> value = SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&RPKFileOptions)
			.OnGenerateWidget(this, &FPRTDetail::GetComboOption)
			.OnSelectionChanged(this, &FPRTDetail::HandleRPKFileChanged)
			.InitiallySelectedItem(RPKFileSelected)[
			SNew(STextBlock)
			.Text(FText::FromString(*Selected))
		];
		RPKFileSelector = value;
		if (RPKFileSelector.IsValid())
		{
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

			//build the final row
			DetailBuilderPtr->EditCategory("RPK File", FText::GetEmpty(), ECategoryPriority::Important)
			                .AddCustomRow(FText::FromString("RPK File"))
			                .NameContent()
				[
					SNew(STextBlock).Text(FText::FromString("RPK File:"))
				]
				.ValueContent()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				[
					ValueContent
				];
		}
	}
}

void FPRTDetail::AddOBJFileSelector()
{
	APRTActor* PrtActor = PRTActor.Get();
	const FString Selected = PrtActor->OBJFile;

	//only rebuild if never built before.
	if (OBJFileOptions.Num() == 0)
	{
		for (int32 i = 0; i < PrtActor->OBJFiles.Num(); i++)
		{
			TSharedPtr<FString> temp = MakeShareable(new FString(PrtActor->OBJFiles[i].Name));
			OBJFileOptions.Add(temp);
			if (PrtActor->OBJFiles[i].Name.Compare(Selected) == 0)
			{
				OBJFileSelected = temp;
			}
		}
		//just in case.
		if (OBJFileOptions.Num() > 0 && !OBJFileSelected.IsValid())
		{
			OBJFileSelected = OBJFileOptions[0];
		}
	}
	if (OBJFileOptions.Num() > 0 && OBJFileSelected.IsValid())
	{
		TSharedRef<SComboBox<TSharedPtr<FString>>> value = SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&OBJFileOptions)
			.OnGenerateWidget(this, &FPRTDetail::GetComboOption)
			.OnSelectionChanged(this, &FPRTDetail::HandleOBJFileChanged)
			.InitiallySelectedItem(OBJFileSelected)[
			SNew(STextBlock)
			.Text(FText::FromString(*Selected))
		];
		OBJFileSelector = value;
		if (OBJFileSelector.IsValid())
		{
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

			//build the final row
			DetailBuilderPtr->EditCategory("RPK File", FText::GetEmpty(), ECategoryPriority::Important)
			                .AddCustomRow(FText::FromString("Initial Shape File"))
			                .NameContent()
				[
					SNew(STextBlock).Text(FText::FromString("Initial Shape File:"))
				]
				.ValueContent()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				[
					ValueContent
				];
		}
	}
}

void FPRTDetail::HandleRPKFileChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type)
{
	RPKFileSelected = NewValue;
	APRTActor* PrtActor = PRTActor.Get();
	const FString Selection = *RPKFileSelector->GetSelectedItem();
	for (auto i = 0; i < PrtActor->RPKFiles.Num(); i++)
	{
		if (PrtActor->RPKFiles[i].Name.Compare(Selection) == 0)
		{
			PrtActor->RPKFile = PrtActor->RPKFiles[i].Name;
			PrtActor->RPKPath = PrtActor->RPKFiles[i].Path;
		}
	}
	PrtActor->InitializeRPKData(true);
	PrtActor->Generate();
	//PrtActor->bAttributesUpdated = true;
	if (DetailBuilderPtr != nullptr)
	{
		DetailBuilderPtr->ForceRefreshDetails();
	}
}

void FPRTDetail::HandleOBJFileChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type)
{
	OBJFileSelected = NewValue;
	APRTActor* PrtActor = PRTActor.Get();
	const FString Selection = *OBJFileSelector->GetSelectedItem();
	for (auto i = 0; i < PrtActor->OBJFiles.Num(); i++)
	{
		if (PrtActor->OBJFiles[i].Name.Compare(Selection) == 0)
		{
			PrtActor->OBJFile = PrtActor->OBJFiles[i].Name;
			PrtActor->OBJPath = PrtActor->OBJFiles[i].Path;
		}
	}
	PrtActor->InitializeRPKData(true);
	PrtActor->Generate();	// Must Generate to set-up data structures.
	//PrtActor->bAttributesUpdated = true;
	if (DetailBuilderPtr != nullptr)
	{
		DetailBuilderPtr->ForceRefreshDetails();
	}
}

//buttons
void FPRTDetail::AddGenerateButton()
{
	DetailBuilderPtr->EditCategory("RPK File", FText::GetEmpty(), ECategoryPriority::Important)
	                .AddCustomRow(FText::FromString(L"Regenerate"))
	                .NameContent()[
			SNew(STextBlock)
			.Text(FText::FromString(""))
		]
		.ValueContent()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
			.Text(FText::FromString("Regenerate"))
			.OnClicked(this, &FPRTDetail::HandleGenerateClicked)
			]
			.VAlign(VAlign_Fill)
		];

	
}


void FPRTDetail::AddWriteToDiskButton()
{
#ifdef WRITE_TO_DISK_BUTTON
	APRTActor* PrtActor = PRTActor.Get();

	DetailBuilderPtr->EditCategory("RPK Utility", FText::GetEmpty(), ECategoryPriority::Important)
	                .AddCustomRow(FText::FromString(L"Write OBJ File to Disk"))
	                .NameContent()[
			SNew(STextBlock)
			.Text(FText::FromString(""))
		]
		.ValueContent()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
			.Text(FText::FromString("Create Static Mesh"))
			.OnClicked(this, &FPrtDetail::HandleWriteToDiskClicked)
			]
			.VAlign(VAlign_Fill)
		];
#endif
}

FReply FPRTDetail::HandleWriteToDiskClicked() const
{
#ifdef WRITE_TO_DISK_BUTTON
	APRTActor* PrtActor = PRTActor->Get();

	PrtActor->CreateStaticMesh();
	PrtActor->WriteToDisk();
#endif
	return FReply::Handled();
}

/****************************************************************************************************************
*
*		The main:
*		This method is called when the detail builder is initialized by the plugin, and whenever force
*		refresh is called. This begins the rendering process of the attributes.
*
****************************************************************************************************************/
void FPRTDetail::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilderPtr = &DetailBuilder;

	// Set the PRTActor so we can access within this class.
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;
	DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);

	for (int32 ObjectIndex = 0; !PRTActor.IsValid() && ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
		if (CurrentObject.IsValid())
		{
			PRTActor = Cast<APRTActor>(CurrentObject.Get());
		}
	}

	//and in return set the actors copy of this, so it can access this class.
	APRTActor* PrtActor = PRTActor.Get();
	PrtActor->PrtDetail = this;

	///just test values should be removed
	FString TestText = PRTActor.Get()->RPKFile;

	AddRPKFileSelector();
	AddOBJFileSelector();

	if (PrtActor->RPKPath.Len() > 0)
	{
		AddGenerateButton();
	}
	
	/// Where the actual detail panel customization starts
	TArray<FCEGroup>& Groups = PRTActor.Get()->ViewAttributes;
	for (int32 i = 0; i < Groups.Num(); i++)
	{
		FString Group = L"RPK Attributes";
		if (Groups[i].Name.Len() > 0)
		{
			Group = L"RPK Attributes: " + Groups[i].Name;
		}
		for (auto j = 0; j < Groups[i].Attributes.Num(); j++)
		{
			FCEAttribute& Attribute = Groups[i].Attributes[j];
			Attribute.SlateWidget.SetAttribute(i, j, Attribute, PrtActor, DetailBuilderPtr, Group);
			Attribute.SlateWidget.Create();
		}
	}
	if (PrtActor->RPKPath.Len() > 0)
	{
		AddWriteToDiskButton();
	}
}


#endif
