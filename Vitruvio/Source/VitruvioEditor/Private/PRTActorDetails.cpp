#include "PRTActorDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "IDetailGroup.h"
#include "PRTActor.h"
#include "Algo/Transform.h"
#include "Widgets/Colors/SColorPicker.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Colors/SColorBlock.h"

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
		return *In ? L"True" : L"False";
	}

	template <typename A, typename V>
	void UpdateAttributeValue(APRTActor* PrtActor, A* Attribute, const V& Value)
	{
		Attribute->Value = Value;
		if (PrtActor->GenerateAutomatically)
		{
			PrtActor->Regenerate();
		}
	}
	
	template <typename A, typename V>
	TSharedPtr<SPropertyComboBox<V>> CreateEnumWidget(A* Attribute, TSharedPtr<EnumAnnotation<V>> Annot, APRTActor* PrtActor)
	{
		TArray<TSharedPtr<V>> SharedPtrValues;
		Algo::Transform(Annot->Values, SharedPtrValues, [](const V& Value) {return MakeShared<V>(Value); });
		auto InitialSelectedIndex = Annot->Values.IndexOfByPredicate([Attribute](const V& Value) {return Value == Attribute->Value; });
		auto InitialSelectedValue = InitialSelectedIndex != INDEX_NONE ? SharedPtrValues[InitialSelectedIndex] : nullptr;

		auto ValueWidget = SNew(SPropertyComboBox<V>)
			.ComboItemList(SharedPtrValues)
			.OnSelectionChanged_Lambda([PrtActor, Attribute](TSharedPtr<V> Val, ESelectInfo::Type Type)
			{
				UpdateAttributeValue(PrtActor, Attribute, *Val);
			})
			.InitialValue(InitialSelectedValue);
		
		return ValueWidget;
	}

	void CreateColorPicker(UStringAttribute* Attribute, APRTActor* PrtActor)
	{
		FColorPickerArgs PickerArgs;
		{
			PickerArgs.bUseAlpha = false;
			PickerArgs.bOnlyRefreshOnOk = true;
			PickerArgs.sRGBOverride = true;
			PickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
			PickerArgs.InitialColorOverride = FLinearColor(FColor::FromHex(Attribute->Value));
			PickerArgs.OnColorCommitted.BindLambda([Attribute, PrtActor](FLinearColor NewColor)
				{
					UpdateAttributeValue(PrtActor, Attribute, L"#" + NewColor.ToFColor(true).ToHex());
				});
		}

		OpenColorPicker(PickerArgs);
	}
	
	TSharedPtr<SHorizontalBox> CreateColorInputWidget(UStringAttribute* Attribute, APRTActor* PrtActor)
	{
				
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
				.OnMouseButtonDown_Lambda([Attribute, PrtActor](const FGeometry& Geometry, const FPointerEvent& Event) -> FReply
				{
					if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
					{
						return FReply::Unhandled();
					}

					CreateColorPicker(Attribute, PrtActor);
					return FReply::Handled();
				})
				.UseSRGB(true)
				.IgnoreAlpha(true)
				.Size(FVector2D(35.0f, 12.0f))
			];
	}
	
	
	TSharedPtr<SCheckBox> CreateBoolInputWidget(UBoolAttribute* Attribute, APRTActor* PrtActor)
	{
		auto OnCheckStateChanged = [PrtActor, Attribute](ECheckBoxState CheckBoxState) -> void
		{
			UpdateAttributeValue(PrtActor, Attribute, CheckBoxState == ECheckBoxState::Checked);
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
			UpdateAttributeValue(PrtActor, Attribute, Text.ToString());
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
		auto Annotation = Attribute->GetRangeAnnotation();
		auto OnCommit = [PrtActor, Attribute](double Value, ETextCommit::Type Type) -> void
		{
			UpdateAttributeValue(PrtActor, Attribute, Value);
		};
		
		auto ValueWidget = SNew(SSpinBox<double>)
            .Font(IDetailLayoutBuilder::GetDetailFont())
			.MinValue(Annotation ? Annotation->Min : TOptional<double>())
			.MaxValue(Annotation ? Annotation->Max : TOptional<double>())
			.OnValueCommitted_Lambda(OnCommit)
			.SliderExponent(1);

		if (Annotation)
		{
			ValueWidget->SetDelta(Annotation->StepSize);
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

	IDetailGroup* GetOrCreateGroups(IDetailGroup& Root, const FAttributeGroups& Groups, TMap<FString, IDetailGroup*>& GroupCache)
	{
		if (Groups.Num() == 0)
		{
			return &Root;
		}

		auto GetOrCreateGroup = [&GroupCache](IDetailGroup& Parent, FString Name) -> IDetailGroup*
		{
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

	void BuildAttributeEditor(IDetailLayoutBuilder& DetailBuilder, APRTActor* PrtActor)
	{
		if (!PrtActor || !PrtActor->Rpk)
		{
			return;
		}

		IDetailCategoryBuilder& RootCategory = DetailBuilder.EditCategory("CGA");
		RootCategory.SetShowAdvanced(true);

		IDetailGroup& RootGroup = RootCategory.AddGroup("Attributes", FText::FromString("Attributes"), true, true);
		TMap<FString, IDetailGroup*> GroupCache;

		for (const auto& AttributeEntry : PrtActor->Attributes)
		{
			URuleAttribute* Attribute = AttributeEntry.Value;

			IDetailGroup* Group = GetOrCreateGroups(RootGroup, Attribute->Groups, GroupCache);
			FDetailWidgetRow& Row = Group->AddWidgetRow();

			Row.NameContent()[CreateNameWidget(Attribute).ToSharedRef()];

			if (UFloatAttribute* FloatAttribute = Cast<UFloatAttribute>(Attribute))
			{
				if (FloatAttribute->GetEnumAnnotation())
				{
					Row.ValueContent()[CreateEnumWidget(FloatAttribute, FloatAttribute->GetEnumAnnotation(), PrtActor).ToSharedRef()];
				}
				else
				{
					Row.ValueContent()[CreateNumericInputWidget(FloatAttribute, PrtActor).ToSharedRef()];
				}
			}
			else if (UStringAttribute* StringAttribute = Cast<UStringAttribute>(Attribute))
			{
				if (StringAttribute->GetEnumAnnotation())
				{
					Row.ValueContent()[CreateEnumWidget(StringAttribute, StringAttribute->GetEnumAnnotation(), PrtActor).ToSharedRef()];
				}
				else if (StringAttribute->GetColorAnnotation())
				{
					Row.ValueContent()[CreateColorInputWidget(StringAttribute, PrtActor).ToSharedRef()];
				}
				else
				{
					Row.ValueContent()[CreateTextInputWidget(StringAttribute, PrtActor).ToSharedRef()];
				}
			}
			else if (UBoolAttribute* BoolAttribute = Cast<UBoolAttribute>(Attribute))
			{
				Row.ValueContent()[CreateBoolInputWidget(BoolAttribute, PrtActor).ToSharedRef()];
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
			.Font(IDetailLayoutBuilder::GetDetailFont())
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

