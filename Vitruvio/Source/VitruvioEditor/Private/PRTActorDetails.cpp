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
	TSharedPtr<SHorizontalBox> CreateTextInputWidget(UStringAttribute* Attribute, APRTActor* PrtActor)
	{
		auto OnTextChanged = [PrtActor, Attribute](const FText& Text, ETextCommit::Type) -> void
		{
			Attribute->Value = Text.ToString();
			PrtActor->Regenerate();
		};

		auto ValueWidget = SNew(SEditableTextBox)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.IsReadOnly(false)
			.SelectAllTextWhenFocused(true)
			.OnTextCommitted_Lambda(OnTextChanged);

		ValueWidget->SetText(FText::FromString(Attribute->Value));

		
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			.FillWidth(1)
			[
				ValueWidget
			];
	}
	
	TSharedPtr<SSpinBox<double>> CreateNumericInputWidget(UFloatAttribute* Attribute, APRTActor* PrtActor)
	{
		URangeAnnotation* RangeAnnotation = Cast<URangeAnnotation>(Attribute->Metadata->Annotation);
		const TOptional<double> MaxValue = RangeAnnotation ? RangeAnnotation->Max : TOptional<double>();
		const TOptional<double> MinValue = RangeAnnotation ? RangeAnnotation->Min : TOptional<double>();

		auto OnCommit = [PrtActor, Attribute](double Value, ETextCommit::Type Type) -> void
		{
			Attribute->Value = Value;
			PrtActor->Regenerate();
		};
		
		auto ValueWidget = SNew(SSpinBox<double>)
            .Font(IDetailLayoutBuilder::GetDetailFont())
			.MinValue(MinValue)
			.MaxValue(MaxValue)
			.OnValueCommitted_Lambda(OnCommit)
			.SliderExponent(1);

		if (RangeAnnotation)
		{
			ValueWidget->SetDelta(RangeAnnotation->StepSize);
		}
		
		ValueWidget->SetValue(Attribute->Value);
		
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
	
	void BuildAttributeEditor(IDetailLayoutBuilder& DetailBuilder, APRTActor* PrtActor)
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
				Row.ValueContent() [ CreateNumericInputWidget(FloatAttribute, PrtActor).ToSharedRef() ];
			}
			else if (UStringAttribute* StringAttribute = Cast<UStringAttribute>(Attribute))
			{
				Row.ValueContent()[CreateTextInputWidget(StringAttribute, PrtActor).ToSharedRef()];
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

