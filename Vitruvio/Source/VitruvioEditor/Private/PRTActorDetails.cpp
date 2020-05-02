#include "PRTActorDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "PRTActor.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSpinBox.h"

namespace
{
	TSharedPtr<SSpinBox<double>> CreateNumericInputWidget(UFloatAttribute* Attribute)
	{
		auto ValueWidget = SNew(SSpinBox<double>)
            .Font(IDetailLayoutBuilder::GetDetailFont());

		URangeAnnotation* RangeAnnotation = Cast<URangeAnnotation>(Attribute->Metadata->Annotation);
		if (RangeAnnotation)
		{
			ValueWidget->SetMinValue(RangeAnnotation->Min);
			ValueWidget->SetMaxValue(RangeAnnotation->Max);
			ValueWidget->SetDelta(RangeAnnotation->StepSize);
		}

		return ValueWidget;
	}
	
	TSharedPtr<SBox> CreateNameWidget(URuleAttribute* Attribute)
	{
		auto NameWidget = SNew(SBox)
		.Content()
           [
               SNew(STextBlock)
               .Text(FText::FromString(Attribute->DisplayName))
               .Font(IDetailLayoutBuilder::GetDetailFont())
           ];
		return NameWidget;
	}
	
	void BuildAttributeEditor(IDetailLayoutBuilder& DetailBuilder, const APRTActor* PrtActor)
	{
		if (!PrtActor || !PrtActor->Rpk) return;

		IDetailCategoryBuilder& RootCategory = DetailBuilder.EditCategory("AttributesRoot", FText::FromString(PrtActor->Rpk->GetName()));
		for (const auto& AttributeEntry : PrtActor->Attributes)
		{
			URuleAttribute* Attribute = AttributeEntry.Value;
			FDetailWidgetRow& Row = RootCategory.AddCustomRow(FText::FromString(Attribute->Name));

			Row.NameContent() [ CreateNameWidget(Attribute).ToSharedRef() ];
			if (UFloatAttribute* FloatAttribute = Cast<UFloatAttribute>(Attribute))
			{
				Row.ValueContent() [ CreateNumericInputWidget(FloatAttribute).ToSharedRef() ];
			}
		}
		
	}
}

FPRTActorDetails::FPRTActorDetails()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FPRTActorDetails::OnAttributesChanged);
}

FPRTActorDetails::~FPRTActorDetails()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

TSharedRef<IDetailCustomization> FPRTActorDetails::MakeInstance()
{
	return MakeShareable(new FPRTActorDetails);
}

void FPRTActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	ObjectsBeingCustomized.Empty();
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);
	
	APRTActor* PrtActor = nullptr;
	for (size_t ObjectIndex = 0; ObjectIndex < ObjectsBeingCustomized.Num(); ++ObjectIndex)
	{
		const TWeakObjectPtr<UObject>& CurrentObject = ObjectsBeingCustomized[ObjectIndex];
		if (CurrentObject.IsValid())
		{
			PrtActor = Cast<APRTActor>(CurrentObject.Get());
		}
	}

	if (!PrtActor) return;

	DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(APRTActor, Attributes))->MarkHiddenByCustomization();
	
	if (PrtActor)
	{
		BuildAttributeEditor(DetailBuilder, PrtActor);
	}
}

void FPRTActorDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	CachedDetailBuilder = DetailBuilder;
	CustomizeDetails(*DetailBuilder);
}

void FPRTActorDetails::OnAttributesChanged(UObject* Object, struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(APRTActor, Attributes))
	{
		const auto DetailBuilder = CachedDetailBuilder.Pin().Get();
		if (DetailBuilder)
		{
			DetailBuilder->ForceRefreshDetails();
		}
	}
}

