// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "VitruvioComponentDetails.h"

#include "UnrealEd.h"
#include "VitruvioComponent.h"

#include "Algo/Transform.h"
#include "Brushes/SlateColorBrush.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "LevelEditor.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
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

bool IsVitruvioComponentSelected(const TArray<TWeakObjectPtr<UObject>>& ObjectsBeingCustomized, UVitruvioComponent*& OutComponent)
{
	OutComponent = nullptr;
	for (size_t ObjectIndex = 0; ObjectIndex < ObjectsBeingCustomized.Num(); ++ObjectIndex)
	{
		const TWeakObjectPtr<UObject>& CurrentObject = ObjectsBeingCustomized[ObjectIndex];
		if (CurrentObject.IsValid())
		{
			UVitruvioComponent* VitruvioComponent = Cast<UVitruvioComponent>(CurrentObject.Get());
			if (VitruvioComponent)
			{
				OutComponent = VitruvioComponent;
				return true;
			}
		}
	}
	return false;
}

template <typename Attr, typename V, typename An>
TSharedPtr<SPropertyComboBox<V>> CreateEnumWidget(Attr* Attribute, An* Annotation, UVitruvioComponent* VitruvioActor)
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
		.MinValue(Annotation && !FMath::IsNaN(Annotation->Min) ? Annotation->Min : TOptional<double>())
		.MaxValue(Annotation && !FMath::IsNaN(Annotation->Max) ? Annotation->Max : TOptional<double>())
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

IDetailGroup* GetOrCreateGroups(IDetailGroup& Root, const TArray<FString>& Groups, TMap<FString, IDetailGroup*>& GroupCache)
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

void AddSeparator(IDetailCategoryBuilder& RootCategory)
{
	// clang-format off
	RootCategory.AddCustomRow(FText::FromString(L"Divider"), true).WholeRowContent()
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Fill)
	[
		SNew(SSeparator)
		.Orientation(Orient_Horizontal)
		.Thickness(0.5f)
		.SeparatorImage(new FSlateColorBrush(FLinearColor(FColor(47, 47, 47))))
	];
	// clang-format on
}

void BuildAttributeEditor(IDetailCategoryBuilder& RootCategory, UVitruvioComponent* VitruvioActor)
{
	if (!VitruvioActor || !VitruvioActor->GetRpk())
	{
		return;
	}

	IDetailGroup& RootGroup = RootCategory.AddGroup("Attributes", FText::FromString("Attributes"), true, true);
	TMap<FString, IDetailGroup*> GroupCache;

	for (const auto& AttributeEntry : VitruvioActor->GetAttributes())
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
				Row.ValueContent()[CreateEnumWidget<UFloatAttribute, double, UFloatEnumAnnotation>(FloatAttribute,
																								   FloatAttribute->GetEnumAnnotation(), VitruvioActor)
									   .ToSharedRef()];
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
				Row.ValueContent()[CreateEnumWidget<UStringAttribute, FString, UStringEnumAnnotation>(
									   StringAttribute, StringAttribute->GetEnumAnnotation(), VitruvioActor)
									   .ToSharedRef()];
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

void AddGenerateButton(IDetailCategoryBuilder& RootCategory, UVitruvioComponent* VitruvioComponent)
{
	// clang-format off
	RootCategory.AddCustomRow(FText::FromString(L"Generate"), true)
	.WholeRowContent()
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SButton)
			.Text(FText::FromString("Generate"))
			.ContentPadding(FMargin(30, 2))
			.OnClicked_Lambda([VitruvioComponent]()
			{
				VitruvioComponent->Generate();
				return FReply::Handled();
			})
		]
		.VAlign(VAlign_Fill)
	];
	// clang-format on
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
	for (const auto& InitialShapeType : UVitruvioComponent::GetInitialShapesClasses())
	{
		FString DisplayName = InitialShapeType->GetMetaData(TEXT("DisplayName"));

		auto InitialShapeOption = MakeShared<FString>(DisplayName);
		InitialShapeTypes.Add(InitialShapeOption);
		InitialShapeTypeMap.Add(InitialShapeOption, *InitialShapeType);
	}

	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FVitruvioComponentDetails::OnPropertyChanged);
	UVitruvioComponent::OnHierarchyChanged.AddRaw(this, &FVitruvioComponentDetails::OnVitruvioComponentHierarchyChanged);
}

FVitruvioComponentDetails::~FVitruvioComponentDetails()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

TSharedRef<IDetailCustomization> FVitruvioComponentDetails::MakeInstance()
{
	return MakeShareable(new FVitruvioComponentDetails);
}

void FVitruvioComponentDetails::AddSwitchInitialShapeCombobox(IDetailCategoryBuilder& RootCategory,
															  const TSharedPtr<FString>& CurrentInitialShapeType,
															  UVitruvioComponent* VitruvioComponent)
{
	// clang-format off
	FDetailWidgetRow& Row = RootCategory.AddCustomRow(FText::FromString("InitialShape"), false);

	Row.NameContent()
	[
		SNew(SBox)
		.Content()
		[
			SNew(STextBlock)
			.Text(FText::FromString("Initial Shape Type"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
	];

	Row.ValueContent()
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Left)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(STextComboBox)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.InitiallySelectedItem(CurrentInitialShapeType)
			.OnSelectionChanged_Lambda([this, VitruvioComponent](TSharedPtr<FString> Selection, ESelectInfo::Type SelectInfo)
			{
				if (Selection.IsValid())
				{
					VitruvioComponent->SetInitialShapeType(InitialShapeTypeMap[Selection]);
					VitruvioComponent->Generate();

					// Hack to refresh the property editor
					GEditor->SelectActor(VitruvioComponent->GetOwner(), false, true, true, true);
					GEditor->SelectActor(VitruvioComponent->GetOwner(), true, true, true, true);
					GEditor->SelectComponent(VitruvioComponent, true, true, true);
				}
			})
			.OptionsSource(&InitialShapeTypes)
		]
	];
	// clang-format on
}

void FVitruvioComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	ObjectsBeingCustomized.Empty();
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	// If there are more than one selected items we only hide the attributes and return
	// No support for editing attributes on multiple initial shapes simultaneous
	if (ObjectsBeingCustomized.Num() > 1)
	{
		DetailBuilder.GetProperty(FName(TEXT("Attributes")))->MarkHiddenByCustomization();
		return;
	}

	UVitruvioComponent* VitruvioComponent = nullptr;

	if (IsVitruvioComponentSelected(ObjectsBeingCustomized, VitruvioComponent))
	{
		DetailBuilder.GetProperty(FName(TEXT("Attributes")))->MarkHiddenByCustomization();

		if (!VitruvioComponent->InitialShape)
		{
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UVitruvioComponent, InitialShape))->MarkHiddenByCustomization();
		}

		IDetailCategoryBuilder& RootCategory = DetailBuilder.EditCategory("Vitruvio");
		RootCategory.SetShowAdvanced(true);

		if (!VitruvioComponent->GenerateAutomatically)
		{
			AddGenerateButton(RootCategory, VitruvioComponent);
		}

		if (VitruvioComponent->InitialShape && VitruvioComponent->InitialShape->CanDestroy())
		{
			TSharedPtr<FString> CurrentInitialShapeType;

			if (VitruvioComponent->InitialShape)
			{
				for (const auto& InitialShapeTypeOptionTuple : InitialShapeTypeMap)
				{
					UClass* IsClass = VitruvioComponent->InitialShape->GetClass();
					if (InitialShapeTypeOptionTuple.Value == IsClass)
					{
						CurrentInitialShapeType = InitialShapeTypeOptionTuple.Key;
						break;
					}
				}
			}

			AddSwitchInitialShapeCombobox(RootCategory, CurrentInitialShapeType, VitruvioComponent);
		}

		AddSeparator(RootCategory);

		BuildAttributeEditor(RootCategory, VitruvioComponent);
	}
}

void FVitruvioComponentDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	CachedDetailBuilder = DetailBuilder;
	CustomizeDetails(*DetailBuilder);
}

void FVitruvioComponentDetails::OnPropertyChanged(UObject* Object, struct FPropertyChangedEvent& Event)
{
	if (!Event.Property || Event.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	const FName PropertyName = Event.Property->GetFName();
	if (PropertyName == FName(TEXT("Attributes")) || PropertyName == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, GenerateAutomatically))
	{
		const auto DetailBuilder = CachedDetailBuilder.Pin().Get();

		if (!DetailBuilder)
		{
			return;
		}

		TArray<TWeakObjectPtr<UObject>> Objects;
		DetailBuilder->GetObjectsBeingCustomized(Objects);

		if (Objects.Num() > 0)
		{
			return;
		}

		UObject* ObjectModified = Objects[0].Get();
		UVitruvioComponent* Component = Cast<UVitruvioComponent>(Object);
		AActor* Owner = nullptr;
		if (Component)
		{
			Owner = Component->GetOwner();
		}

		if (ObjectModified == Component || ObjectModified == Owner)
		{
			DetailBuilder->ForceRefreshDetails();
		}
	}
}

void FVitruvioComponentDetails::OnVitruvioComponentHierarchyChanged(UVitruvioComponent* Component)
{
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnComponentsEdited().Broadcast();
}