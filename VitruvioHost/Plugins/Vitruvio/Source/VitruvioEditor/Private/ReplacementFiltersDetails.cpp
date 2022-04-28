// Copyright Epic Games, Inc. All Rights Reserved.
#include "ReplacementFiltersDetails.h"
#include "Algo/Accumulate.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailsUtil.h"
#include "IDetailChildrenBuilder.h"
#include "VitruvioReplacements.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "ZoneGraphSettings.h"

#define LOCTEXT_NAMESPACE "VitruvioEditor"

TSharedRef<IPropertyTypeCustomization> FReplacementFiltersDetails::MakeInstance()
{
	return MakeShareable(new FReplacementFiltersDetails);
}

FText FReplacementFiltersDetails::GetFilterText() const
{
	const auto ReplacementFiltersOptional = GetValue<FReplacementFilters>(StructProperty->AsShared());

	FText FilterText;
	if (ReplacementFiltersOptional.IsSet())
	{
		const auto ReplacementFilters = ReplacementFiltersOptional.GetValue();
		TArray<FText> FilterStrings;
		Algo::Transform(ReplacementFilters.Filters, FilterStrings,
						[](const FReplacementFilter& Filter) { return FText::FromString(Filter.ToString()); });
		FilterText = FText::Join(FText::FromString(ReplacementFilters.Type == FilterType::All ? TEXT(" AND ") : TEXT(" OR ")), FilterStrings);
	}

	return FilterText;
}

void FReplacementFiltersDetails::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow,
												 IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructProperty = StructPropertyHandle;

	HeaderRow.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()].ValueContent()
		[SNew(SHorizontalBox)
		 // Description
		 + SHorizontalBox::Slot()
			   .AutoWidth()
			   .VAlign(VAlign_Center)
			   .Padding(FMargin(
				   0.0f, 2.0f, 6.0f,
				   2.0f))[SNew(STextBlock).Text(this, &FReplacementFiltersDetails::GetFilterText).Font(IDetailLayoutBuilder::GetDetailFont())] +
		 SHorizontalBox::Slot().Padding(FMargin(12.0f, 0.0f)).HAlign(HAlign_Right)[StructPropertyHandle->CreateDefaultPropertyButtonWidgets()]];
}

void FReplacementFiltersDetails::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle,
												   class IDetailChildrenBuilder& StructBuilder,
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