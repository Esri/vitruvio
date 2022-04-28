// Copyright Epic Games, Inc. All Rights Reserved.

#include "ReplacementDetails.h"
#include "Algo/Accumulate.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailsUtil.h"
#include "IDetailChildrenBuilder.h"
#include "VitruvioReplacements.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

#define LOCTEXT_NAMESPACE "VitruvioEditor"

TSharedRef<IPropertyTypeCustomization> FReplacementDetails::MakeInstance()
{
	return MakeShareable(new FReplacementDetails);
}

FSlateColor FReplacementDetails::GetTextColor() const
{
	const auto ReplacementOptional = GetValue<FReplacement>(StructProperty->AsShared());

	return !ReplacementOptional.IsSet() || !ReplacementOptional.GetValue().IsValid() ? FSlateColor(FLinearColor(1.0, 0.05, 0.05, 1.0))
																					 : FSlateColor::UseForeground();
}

FText FReplacementDetails::GetReplacementText() const
{
	const auto ReplacementOptional = GetValue<FReplacement>(StructProperty->AsShared());

	if (!ReplacementOptional.IsSet() || !ReplacementOptional.GetValue().IsValid())
	{
		return FText::FromString("Invalid");
	}

	const auto Replacement = ReplacementOptional.GetValue();
	TArray<FText> FilterStrings;
	Algo::Transform(Replacement.Filters.Filters, FilterStrings, [](const FReplacementFilter& Filter) { return FText::FromString(Filter.Value); });
	return FText::Join(FText::FromString(TEXT(", ")), FilterStrings);
}

void FReplacementDetails::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow,
										  IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructProperty = StructPropertyHandle;

	HeaderRow.WholeRowContent()
		[SNew(SHorizontalBox)
		 // Description
		 + SHorizontalBox::Slot()
			   .AutoWidth()
			   .VAlign(VAlign_Center)
			   .Padding(FMargin(0.0f, 2.0f, 6.0f, 2.0f))[SNew(STextBlock)
															 .Text(this, &FReplacementDetails::GetReplacementText)
															 .Font(IDetailLayoutBuilder::GetDetailFontBold())
															 .ColorAndOpacity(this, &FReplacementDetails::GetTextColor)] +
		 SHorizontalBox::Slot().Padding(FMargin(12.0f, 0.0f)).HAlign(HAlign_Right)[StructPropertyHandle->CreateDefaultPropertyButtonWidgets()]];
}

void FReplacementDetails::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder,
											IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	uint32 ChildNum = 0;
	if (StructPropertyHandle->GetNumChildren(ChildNum) == FPropertyAccess::Success)
	{
		for (uint32 ChildIndex = 0; ChildIndex < ChildNum; ++ChildIndex)
		{
			TSharedPtr<IPropertyHandle> ChildProperty = StructPropertyHandle->GetChildHandle(ChildIndex);
			if (ChildProperty)
			{
				StructBuilder.AddProperty(ChildProperty.ToSharedRef());
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE