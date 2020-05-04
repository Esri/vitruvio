#include "PRTActorDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "PRTActor.h"
#include "Algo/Transform.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"

namespace
{
	FString ValueToString(const TSharedPtr<FString>& In)
	{
		return *In;
	}

	FString ValueToString(const TSharedPtr<double>& In)
	{
		return(FString::SanitizeFloat(*In));
	}
	
	template <typename A, typename V>
	TSharedPtr<SPropertyComboBox<V>> CreateEnumWidget(A* Attribute, const TArray<V>& Values, APRTActor* PrtActor)
	{
		TArray<TSharedPtr<V>> SharedPtrValues;
		Algo::Transform(Values, SharedPtrValues, [](const V& Value) {return MakeShared<V>(Value); });
		auto InitialSelectedIndex = Values.IndexOfByPredicate([Attribute](const V& Value) {return Value == Attribute->Value; });
		auto InitialSelectedValue = InitialSelectedIndex != INDEX_NONE ? SharedPtrValues[InitialSelectedIndex] : nullptr;

		auto ValueWidget = SNew(SPropertyComboBox<V>)
			.ComboItemList(SharedPtrValues)
			.OnSelectionChanged_Lambda([PrtActor, Attribute](TSharedPtr<V> Val, ESelectInfo::Type Type)
			{
				Attribute->Value = *Val;
				PrtActor->Regenerate();
			})
			.InitialValue(InitialSelectedValue);
		
		return ValueWidget;
	}
	
	TSharedPtr<SCheckBox> CreateBoolInputWidget(UBoolAttribute* Attribute, APRTActor* PrtActor)
	{
		auto OnCheckStateChanged = [PrtActor, Attribute](ECheckBoxState CheckBoxState) -> void
		{
			Attribute->Value = CheckBoxState == ECheckBoxState::Checked;
			PrtActor->Regenerate();
		};

		auto ValueWidget = SNew(SCheckBox)
			.OnCheckStateChanged_Lambda(OnCheckStateChanged);

		ValueWidget->SetIsChecked(Attribute->Value);

		return ValueWidget;
	}
	
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
		auto OnCommit = [PrtActor, Attribute](double Value, ETextCommit::Type Type) -> void
		{
			Attribute->Value = Value;
			PrtActor->Regenerate();
		};
		
		auto ValueWidget = SNew(SSpinBox<double>)
            .Font(IDetailLayoutBuilder::GetDetailFont())
			.MinValue(RangeAnnotation ? RangeAnnotation->Min : TOptional<double>())
			.MaxValue(RangeAnnotation ? RangeAnnotation->Max : TOptional<double>())
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

			if (UEnumAnnotation* EnumAnnotation = Cast<UEnumAnnotation>(Attribute->Metadata->Annotation))
			{
				if (UFloatAttribute* FloatAttribute = Cast<UFloatAttribute>(Attribute))
				{
					Row.ValueContent()[CreateEnumWidget(FloatAttribute, EnumAnnotation->FloatValues, PrtActor).ToSharedRef()];
				}
			}
			else
			{
				if (UFloatAttribute* FloatAttribute = Cast<UFloatAttribute>(Attribute))
				{
					Row.ValueContent()[CreateNumericInputWidget(FloatAttribute, PrtActor).ToSharedRef()];
				}
				else if (UStringAttribute* StringAttribute = Cast<UStringAttribute>(Attribute))
				{
					Row.ValueContent()[CreateTextInputWidget(StringAttribute, PrtActor).ToSharedRef()];
				}
				else if (UBoolAttribute* BoolAttribute = Cast<UBoolAttribute>(Attribute))
				{
					Row.ValueContent()[CreateBoolInputWidget(BoolAttribute, PrtActor).ToSharedRef()];
				}
			}

		}
		
	}
}

template <typename T>
void SPropertyComboBox<T>::Construct(const FArguments& InArgs)
{
	ComboItemList = InArgs._ComboItemList.Get();

	SComboBox<TSharedPtr<T>>::Construct(SComboBox<TSharedPtr<T>>::FArguments()
		.InitiallySelectedItem(InArgs._InitialValue.Get())
		.Content()
		[
			SNew(STextBlock)
			.Text_Lambda([=]
			{
				auto SelectedItem = GetSelectedItem();
				return SelectedItem ? FText::FromString(ValueToString(SelectedItem)) : FText::FromString("");
			})
		]
		.OptionsSource(&ComboItemList)
		.OnSelectionChanged(InArgs._OnSelectionChanged)
		.OnGenerateWidget(this, &SPropertyComboBox::OnGenerateComboWidget)
	);
}

template <typename T>
TSharedRef<SWidget> SPropertyComboBox<T>::OnGenerateComboWidget(TSharedPtr<T> InValue) const
{
	return SNew(STextBlock)
		.Text(FText::FromString(ValueToString(InValue)));
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

