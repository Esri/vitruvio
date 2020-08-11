// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "VitruvioComponentDetails.h"

#include "VitruvioComponent.h"

#include "Algo/Transform.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
FString ValueToString(const TSharedPtr<FString>& In)
{
	return *In;
}

FString ValueToString(const TSharedPtr<double>& In)
{
	return FString::SanitizeFloat(*In);
}

FString ValueToString(const TSharedPtr<bool>& In)
{
	return *In ? TEXT("True") : TEXT("False");
}

template <typename A, typename V>
void UpdateAttributeValue(UVitruvioComponent* VitruvioActor, A* Attribute, const V& Value)
{
	Attribute->Value = Value;
	if (VitruvioActor->GenerateAutomatically)
	{
		VitruvioActor->Generate();
	}
}

template <typename A, typename V>
TSharedPtr<SPropertyComboBox<V>> CreateEnumWidget(A* Attribute, TSharedPtr<EnumAnnotation<V>> Annotation, UVitruvioComponent* VitruvioActor)
{
	TArray<TSharedPtr<V>> SharedPtrValues;
	Algo::Transform(Annotation->Values, SharedPtrValues, [](const V& Value) { return MakeShared<V>(Value); });
	auto InitialSelectedIndex = Annotation->Values.IndexOfByPredicate([Attribute](const V& Value) { return Value == Attribute->Value; });
	auto InitialSelectedValue = InitialSelectedIndex != INDEX_NONE ? SharedPtrValues[InitialSelectedIndex] : nullptr;

	auto ValueWidget = SNew(SPropertyComboBox<V>)
						   .ComboItemList(SharedPtrValues)
						   .OnSelectionChanged_Lambda([VitruvioActor, Attribute](TSharedPtr<V> Val, ESelectInfo::Type Type) {
							   UpdateAttributeValue(VitruvioActor, Attribute, *Val);
						   })
						   .InitialValue(InitialSelectedValue);

	return ValueWidget;
}

void CreateColorPicker(UStringAttribute* Attribute, UVitruvioComponent* VitruvioActor)
{
	FColorPickerArgs PickerArgs;
	{
		PickerArgs.bUseAlpha = false;
		PickerArgs.bOnlyRefreshOnOk = true;
		PickerArgs.sRGBOverride = true;
		PickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
		PickerArgs.InitialColorOverride = FLinearColor(FColor::FromHex(Attribute->Value));
		PickerArgs.OnColorCommitted.BindLambda([Attribute, VitruvioActor](FLinearColor NewColor) {
			UpdateAttributeValue(VitruvioActor, Attribute, TEXT("#") + NewColor.ToFColor(true).ToHex());
		});
	}

	OpenColorPicker(PickerArgs);
}

TSharedPtr<SHorizontalBox> CreateColorInputWidget(UStringAttribute* Attribute, UVitruvioComponent* VitruvioActor)
{
	// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 2.0f)
			[
				// Displays the color without alpha
				SNew(SColorBlock)
				.Color_Lambda([Attribute]()
				{
					return FLinearColor(FColor::FromHex(Attribute->Value));
				})
				.ShowBackgroundForAlpha(false)
				.OnMouseButtonDown_Lambda([Attribute, VitruvioActor](const FGeometry& Geometry, const FPointerEvent& Event) -> FReply
				{
					if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
					{
						return FReply::Unhandled();
					}

					CreateColorPicker(Attribute, VitruvioActor);
					return FReply::Handled();
				})
				.UseSRGB(true)
				.IgnoreAlpha(true)
				.Size(FVector2D(35.0f, 12.0f))
			];
	// clang-format on
}

TSharedPtr<SCheckBox> CreateBoolInputWidget(UBoolAttribute* Attribute, UVitruvioComponent* VitruvioActor)
{
	auto OnCheckStateChanged = [VitruvioActor, Attribute](ECheckBoxState CheckBoxState) -> void {
		UpdateAttributeValue(VitruvioActor, Attribute, CheckBoxState == ECheckBoxState::Checked);
	};

	auto ValueWidget = SNew(SCheckBox).OnCheckStateChanged_Lambda(OnCheckStateChanged);

	ValueWidget->SetIsChecked(Attribute->Value);

	return ValueWidget;
}

TSharedPtr<SHorizontalBox> CreateTextInputWidget(UStringAttribute* Attribute, UVitruvioComponent* VitruvioActor)
{
	auto OnTextChanged = [VitruvioActor, Attribute](const FText& Text, ETextCommit::Type) -> void {
		UpdateAttributeValue(VitruvioActor, Attribute, Text.ToString());
	};

	// clang-format off
		auto ValueWidget = SNew(SEditableTextBox)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.IsReadOnly(false)
			.SelectAllTextWhenFocused(true)
			.OnTextCommitted_Lambda(OnTextChanged);
	// clang-format on

	ValueWidget->SetText(FText::FromString(Attribute->Value));

	// clang-format off
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			.FillWidth(1)
			[
				ValueWidget
			];
	// clang-format on
}

TSharedPtr<SSpinBox<double>> CreateNumericInputWidget(UFloatAttribute* Attribute, UVitruvioComponent* VitruvioActor)
{
	auto Annotation = Attribute->GetRangeAnnotation();
	auto OnCommit = [VitruvioActor, Attribute](double Value, ETextCommit::Type Type) -> void {
		UpdateAttributeValue(VitruvioActor, Attribute, Value);
	};

	// clang-format off
		auto ValueWidget = SNew(SSpinBox<double>)
            .Font(IDetailLayoutBuilder::GetDetailFont())
			.MinValue(Annotation ? Annotation->Min : TOptional<double>())
			.MaxValue(Annotation ? Annotation->Max : TOptional<double>())
			.OnValueCommitted_Lambda(OnCommit)
			.SliderExponent(1);
	// clang-format on

	if (Annotation)
	{
		ValueWidget->SetDelta(Annotation->StepSize);
	}

	ValueWidget->SetValue(Attribute->Value);

	return ValueWidget;
}

TSharedPtr<SBox> CreateNameWidget(URuleAttribute* Attribute)
{
	// clang-format off
		auto NameWidget = SNew(SBox)
			.Content()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Attribute->DisplayName))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			];
	// clang-format on
	return NameWidget;
}

IDetailGroup* GetOrCreateGroups(IDetailGroup& Root, const FAttributeGroups& Groups, TMap<FString, IDetailGroup*>& GroupCache)
{
	if (Groups.Num() == 0)
	{
		return &Root;
	}

	auto GetOrCreateGroup = [&GroupCache](IDetailGroup& Parent, FString Name) -> IDetailGroup* {
		const auto CacheResult = GroupCache.Find(Name);
		if (CacheResult)
		{
			return *CacheResult;
		}
		IDetailGroup& Group = Parent.AddGroup(*Name, FText::FromString(Name), true);
		GroupCache.Add(Name, &Group);
		return &Group;
	};

	FString QualifiedIdentifier = Groups[0];
	IDetailGroup* CurrentGroup = GetOrCreateGroup(Root, QualifiedIdentifier);
	for (auto GroupIndex = 1; GroupIndex < Groups.Num(); ++GroupIndex)
	{
		QualifiedIdentifier += Groups[GroupIndex];
		CurrentGroup = GetOrCreateGroup(*CurrentGroup, Groups[GroupIndex]);
	}

	return CurrentGroup;
}

void BuildInitialShapeEditor(IDetailCategoryBuilder& RootCategory, UVitruvioComponent* VitruvioComponent)
{
	if (!VitruvioComponent || !VitruvioComponent->InitialShape)
	{
		return;
	}

	FInitialShapeFactory* Factory = VitruvioComponent->InitialShapeFactory;
	if (!Factory || !Factory->HasCustomEditor())
	{
		return;
	}

	IDetailGroup& InitialShapeGroup = RootCategory.AddGroup("InitialShape", FText::FromString("Initial Shape"), true, true);

	UClass* Class = VitruvioComponent->InitialShape->GetClass();
	for (TFieldIterator<FProperty> PropertyIterator(Class); PropertyIterator; ++PropertyIterator)
	{
		FProperty* Property = *PropertyIterator;
		FDetailWidgetRow& Row = InitialShapeGroup.AddWidgetRow();

		// clang-format off
		Row.NameContent()[
            SNew(SBox)
            .Content()
            [
                SNew(STextBlock)
                .Text(Property->GetDisplayNameText())
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
        ];
		// clang-format on

		if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
		{
			int32* ValuePtr = IntProperty->ContainerPtrToValuePtr<int32>(VitruvioComponent->InitialShape);
			auto OnCommit = [IntProperty, VitruvioComponent, ValuePtr](int32 Value, ETextCommit::Type Type) -> void {
				IntProperty->SetIntPropertyValue(ValuePtr, static_cast<int64>(Value));
				if (VitruvioComponent->GenerateAutomatically)
				{
					VitruvioComponent->Generate();
				}
			};

			const FString& UIMin = Property->GetMetaData("UIMin");
			const FString& UIMax = Property->GetMetaData("UIMax");

			// clang-format off
			Row.ValueContent()[
				SNew(SSpinBox<int32>)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.MinValue(UIMin.IsEmpty() ? TOptional<int32>() : FCString::Atoi(*UIMin))
					.MaxValue(UIMax.IsEmpty() ? TOptional<int32>() : FCString::Atoi(*UIMax))
					.OnValueCommitted_Lambda(OnCommit)
					.SliderExponent(1)
					.Value(IntProperty->GetPropertyValue(ValuePtr))
		    ];
			// clang-format on
		}
		else
		{
			// TODO implement other property types
		}
	}
}

void BuildAttributeEditor(IDetailCategoryBuilder& RootCategory, UVitruvioComponent* VitruvioActor)
{
	if (!VitruvioActor || !VitruvioActor->Rpk)
	{
		return;
	}

	IDetailGroup& RootGroup = RootCategory.AddGroup("Attributes", FText::FromString("Attributes"), true, true);
	TMap<FString, IDetailGroup*> GroupCache;

	for (const auto& AttributeEntry : VitruvioActor->Attributes)
	{
		URuleAttribute* Attribute = AttributeEntry.Value;

		IDetailGroup* Group = GetOrCreateGroups(RootGroup, Attribute->Groups, GroupCache);
		FDetailWidgetRow& Row = Group->AddWidgetRow();

		Row.FilterTextString = FText::FromString(Attribute->DisplayName);

		Row.NameContent()[CreateNameWidget(Attribute).ToSharedRef()];

		if (UFloatAttribute* FloatAttribute = Cast<UFloatAttribute>(Attribute))
		{
			if (FloatAttribute->GetEnumAnnotation())
			{
				Row.ValueContent()[CreateEnumWidget(FloatAttribute, FloatAttribute->GetEnumAnnotation(), VitruvioActor).ToSharedRef()];
			}
			else
			{
				Row.ValueContent()[CreateNumericInputWidget(FloatAttribute, VitruvioActor).ToSharedRef()];
			}
		}
		else if (UStringAttribute* StringAttribute = Cast<UStringAttribute>(Attribute))
		{
			if (StringAttribute->GetEnumAnnotation())
			{
				Row.ValueContent()[CreateEnumWidget(StringAttribute, StringAttribute->GetEnumAnnotation(), VitruvioActor).ToSharedRef()];
			}
			else if (StringAttribute->GetColorAnnotation())
			{
				Row.ValueContent()[CreateColorInputWidget(StringAttribute, VitruvioActor).ToSharedRef()];
			}
			else
			{
				Row.ValueContent()[CreateTextInputWidget(StringAttribute, VitruvioActor).ToSharedRef()];
			}
		}
		else if (UBoolAttribute* BoolAttribute = Cast<UBoolAttribute>(Attribute))
		{
			Row.ValueContent()[CreateBoolInputWidget(BoolAttribute, VitruvioActor).ToSharedRef()];
		}
	}
}
} // namespace

template <typename T>
void SPropertyComboBox<T>::Construct(const FArguments& InArgs)
{
	ComboItemList = InArgs._ComboItemList.Get();

	// clang-format off
	SComboBox<TSharedPtr<T>>::Construct(typename SComboBox<TSharedPtr<T>>::FArguments()
		.InitiallySelectedItem(InArgs._InitialValue.Get())
		.Content()
		[
			SNew(STextBlock)
			.Text_Lambda([=]
			{
				auto SelectedItem = SComboBox<TSharedPtr<T>>::GetSelectedItem();
				return SelectedItem ? FText::FromString(ValueToString(SelectedItem)) : FText::FromString("");
			})
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.OptionsSource(&ComboItemList)
		.OnSelectionChanged(InArgs._OnSelectionChanged)
		.OnGenerateWidget(this, &SPropertyComboBox::OnGenerateComboWidget)
	);
	// clang-format on
}

template <typename T>
TSharedRef<SWidget> SPropertyComboBox<T>::OnGenerateComboWidget(TSharedPtr<T> InValue) const
{
	return SNew(STextBlock).Text(FText::FromString(ValueToString(InValue)));
}

FVitruvioComponentDetails::FVitruvioComponentDetails()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FVitruvioComponentDetails::OnAttributesChanged);
}

FVitruvioComponentDetails::~FVitruvioComponentDetails()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

TSharedRef<IDetailCustomization> FVitruvioComponentDetails::MakeInstance()
{
	return MakeShareable(new FVitruvioComponentDetails);
}

void FVitruvioComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	ObjectsBeingCustomized.Empty();
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	UVitruvioComponent* VitruvioComponent = nullptr;
	for (size_t ObjectIndex = 0; ObjectIndex < ObjectsBeingCustomized.Num(); ++ObjectIndex)
	{
		const TWeakObjectPtr<UObject>& CurrentObject = ObjectsBeingCustomized[ObjectIndex];
		if (CurrentObject.IsValid())
		{
			VitruvioComponent = Cast<UVitruvioComponent>(CurrentObject.Get());
		}
	}

	if (!VitruvioComponent)
	{
		return;
	}

	DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UVitruvioComponent, Attributes))->MarkHiddenByCustomization();

	IDetailCategoryBuilder& RootCategory = DetailBuilder.EditCategory("Vitruvio");
	RootCategory.SetShowAdvanced(true);

	if (VitruvioComponent)
	{
		BuildAttributeEditor(RootCategory, VitruvioComponent);
		BuildInitialShapeEditor(RootCategory, VitruvioComponent);
	}
}

void FVitruvioComponentDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	CachedDetailBuilder = DetailBuilder;
	CustomizeDetails(*DetailBuilder);
}

void FVitruvioComponentDetails::OnAttributesChanged(UObject* Object, struct FPropertyChangedEvent& Event)
{
	if (Event.Property && Event.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, Attributes))
	{
		const auto DetailBuilder = CachedDetailBuilder.Pin().Get();
		if (DetailBuilder)
		{
			DetailBuilder->ForceRefreshDetails();
		}
	}
}
