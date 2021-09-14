/* Copyright 2021 Esri
 *
 * Licensed under the Apache License Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "VitruvioComponentDetails.h"

#include "UnrealEd.h"
#include "VitruvioComponent.h"

#include "Algo/Transform.h"
#include "Brushes/SlateColorBrush.h"
#include "IDetailGroup.h"
#include "IDetailTreeNode.h"
#include "IPropertyRowGenerator.h"
#include "ISinglePropertyView.h"
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

bool IsDefault(double Value)
{
	return Value == 0.0;
}

bool IsDefault(const FString& Value)
{
	return Value.IsEmpty();
}

template <typename A, typename V>
void UpdateAttributeValue(UVitruvioComponent* VitruvioActor, A* Attribute, const V& Value)
{
	Attribute->Value = Value;
	Attribute->UserSet = true;
	VitruvioActor->EvaluateRuleAttributes(VitruvioActor->GenerateAutomatically);
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

template <typename V, typename An, typename G, typename S>
TSharedPtr<SPropertyComboBox<V>>  CreateEnumWidget(An* Annotation, S Setter, G Getter)
{
	check(Annotation->Values.Num() > 0)
	
	TArray<TSharedPtr<V>> SharedPtrValues;
	Algo::Transform(Annotation->Values, SharedPtrValues, [](const V& Value) { return MakeShared<V>(Value); });

	V CurrentValue = Getter();
	auto InitialSelectedIndex = Annotation->Values.IndexOfByPredicate([&CurrentValue](const V& Value) { return Value == CurrentValue; });
	
	if (InitialSelectedIndex == INDEX_NONE) 
	{
		// If the value is not present in the enum values we insert it at the beginning (similar behavior to CE inspector)
		if (!IsDefault(CurrentValue))
		{
			SharedPtrValues.Insert(MakeShared<V>(CurrentValue), 0);
		}
		InitialSelectedIndex = 0;
	}
	auto InitialSelectedValue = SharedPtrValues[InitialSelectedIndex];

	auto ValueWidget = SNew(SPropertyComboBox<V>)
						   .ComboItemList(SharedPtrValues)
						   .OnSelectionChanged_Lambda(Setter)
						   .InitialValue(InitialSelectedValue);

	return ValueWidget;
}
template <typename Attr, typename V, typename An>
TSharedPtr<SPropertyComboBox<V>> CreateScalarEnumWidget(Attr* Attribute, An* Annotation, UVitruvioComponent* VitruvioActor)
{
	auto Setter = [VitruvioActor, Attribute](TSharedPtr<V> Val, ESelectInfo::Type Type) {
		UpdateAttributeValue(VitruvioActor, Attribute, *Val);
	};
	auto Getter = [Attribute]() {
		return Attribute->Value;
	};
	return CreateEnumWidget<V, An>(Annotation, Setter, Getter);
}

template <typename V, typename An>
TSharedPtr<SPropertyComboBox<V>> CreateArrayEnumWidget(An* Annotation, TSharedPtr<IPropertyHandle> PropertyHandle)
{
	auto Setter = [PropertyHandle](TSharedPtr<V> Val, ESelectInfo::Type Type) {
		PropertyHandle->SetValue(*Val);
	};
	auto Getter = [PropertyHandle]() {
		V CurrentValue;
		PropertyHandle->GetValue(CurrentValue);
		return CurrentValue;
	};
	
	return CreateEnumWidget<V, An>(Annotation, Setter, Getter);
}

template <typename S, typename G>
void CreateColorPicker(S Setter, G Getter)
{
	FColorPickerArgs PickerArgs;
	{
		PickerArgs.bUseAlpha = false;
		PickerArgs.bOnlyRefreshOnOk = true;
		PickerArgs.sRGBOverride = true;
		PickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
		PickerArgs.InitialColorOverride = Getter();
		PickerArgs.OnColorCommitted.BindLambda(Setter);
	}

	OpenColorPicker(PickerArgs);
}

template <typename S, typename G>
TSharedPtr<SHorizontalBox> CreateColorInputWidget(S Setter, G Getter)
{
	// clang-format off
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 2.0f)
		[
			// Displays the color without alpha
			SNew(SColorBlock)
			.Color_Lambda(Getter)
			.ShowBackgroundForAlpha(false)
			.OnMouseButtonDown_Lambda([Setter, Getter](const FGeometry& Geometry, const FPointerEvent& Event) -> FReply
			{
				if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
				{
					return FReply::Unhandled();
				}

				CreateColorPicker(Setter, Getter);
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

template <typename Attr, typename S, typename G>
TSharedPtr<SSpinBox<double>> CreateNumericInputWidget(Attr* Attribute, S Setter, G Getter)
{
	auto Annotation = Attribute->GetRangeAnnotation();

	// clang-format off
	auto ValueWidget = SNew(SSpinBox<double>)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.MinValue(Annotation && Annotation->HasMin ? Annotation->Min : TOptional<double>())
		.MaxValue(Annotation && Annotation->HasMax ? Annotation->Max : TOptional<double>())
		.OnValueCommitted_Lambda(Setter)
		.SliderExponent(1);
	// clang-format on

	if (Annotation)
	{
		ValueWidget->SetDelta(Annotation->StepSize);
	}

	ValueWidget->SetValue(Getter());

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
			.Font(Attribute->UserSet ? IDetailLayoutBuilder::GetDetailFontBold() : IDetailLayoutBuilder::GetDetailFont())
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

void AddArrayWidget(const TArray<TSharedRef<IDetailTreeNode>> DetailTreeNodes, IDetailGroup& Group, URuleAttribute* Attribute, UVitruvioComponent* VitruvioActor)
{
	if (DetailTreeNodes.Num() == 0 || DetailTreeNodes[0]->GetNodeType() != EDetailNodeType::Category)
	{
		return;
	}

	TArray<TSharedRef<IDetailTreeNode>> ArrayRoots;
	DetailTreeNodes[0]->GetChildren(ArrayRoots);

	for (const auto& ArrayRoot : ArrayRoots)
	{
		if (ArrayRoot->GetNodeType() != EDetailNodeType::Item)
		{
			continue;
		}
		
		// Header Row
		const TSharedPtr<IDetailPropertyRow> HeaderPropertyRow = ArrayRoot->GetRow();
		IDetailGroup& ArrayHeader = Group.AddGroup(TEXT(""), FText::GetEmpty(), true);
		FDetailWidgetRow& Row = ArrayHeader.HeaderRow();
		FDetailWidgetRow DefaultWidgetsRow;
		TSharedPtr<SWidget> NameWidget;
		TSharedPtr<SWidget> ValueWidget;
		HeaderPropertyRow->GetDefaultWidgets(NameWidget, ValueWidget, DefaultWidgetsRow, true);
		Row.NameContent()[CreateNameWidget(Attribute).ToSharedRef()];
		Row.ValueContent()[ValueWidget.ToSharedRef()];

		// Value Rows
		TArray<TSharedRef<IDetailTreeNode>> ArrayTreeNodes;
		ArrayRoot->GetChildren(ArrayTreeNodes);
		for (const auto& ChildNode : ArrayTreeNodes)
		{
			const TSharedPtr<IDetailPropertyRow> DetailPropertyRow = ChildNode->GetRow();
			FDetailWidgetRow& ValueRow = ArrayHeader.AddWidgetRow();

			FDetailWidgetRow ArrayDefaultWidgetsRow;
			TSharedPtr<SWidget> ArrayNameWidget;
			TSharedPtr<SWidget> ArrayValueWidget;

			DetailPropertyRow->GetDefaultWidgets(ArrayNameWidget, ArrayValueWidget, ArrayDefaultWidgetsRow, true);
			ValueRow.NameContent()[ArrayNameWidget.ToSharedRef()];

			if (UFloatArrayAttribute* FloatArrayAttribute = Cast<UFloatArrayAttribute>(Attribute))
			{
				if (FloatArrayAttribute->GetEnumAnnotation() && FloatArrayAttribute->Values.Num() > 0)
				{
					ValueRow.ValueContent()[CreateArrayEnumWidget<double, UFloatEnumAnnotation>(
										FloatArrayAttribute->GetEnumAnnotation(), DetailPropertyRow->GetPropertyHandle())
										.ToSharedRef()];
				}
				else
				{
					auto FloatProperty = DetailPropertyRow->GetPropertyHandle();
					auto FloatSetter = [FloatProperty](double Value, ETextCommit::Type Type) {
						FloatProperty->SetValue(Value);
					};
					auto FloatGetter = [FloatProperty]() {
						double Value;
						FloatProperty->GetValue(Value);
						return Value;
					};
					
					ValueRow.ValueContent()[CreateNumericInputWidget(FloatArrayAttribute, FloatSetter, FloatGetter).ToSharedRef()];
				}
			}
			else if (UStringArrayAttribute* StringArrayAttribute = Cast<UStringArrayAttribute>(Attribute))
			{
				if (StringArrayAttribute->GetEnumAnnotation() && StringArrayAttribute->GetEnumAnnotation()->Values.Num() > 0)
				{
					ValueRow.ValueContent()[CreateArrayEnumWidget<FString, UStringEnumAnnotation>(
										StringArrayAttribute->GetEnumAnnotation(), DetailPropertyRow->GetPropertyHandle())
										.ToSharedRef()];
				}
				else if (StringArrayAttribute->GetColorAnnotation())
				{
					auto ColorStringProperty = DetailPropertyRow->GetPropertyHandle();
					auto ColorSetter = [ColorStringProperty](FLinearColor NewColor) {
						ColorStringProperty->SetValue(TEXT("#") + NewColor.ToFColor(true).ToHex());
					};
					auto ColorGetter = [ColorStringProperty]()
					{
						FString Value;
						ColorStringProperty->GetValue(Value);
						return Value.IsEmpty() ? FLinearColor(1, 1, 1) : FLinearColor(FColor::FromHex(Value));
					};
					
					ValueRow.ValueContent()[CreateColorInputWidget(ColorSetter, ColorGetter).ToSharedRef()];
				}
				else
				{
					ValueRow.ValueContent()[ArrayValueWidget.ToSharedRef()];
				}
			}
			else
			{
				ValueRow.ValueContent()[ArrayValueWidget.ToSharedRef()];
			}
			
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

	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FVitruvioComponentDetails::OnAttributesChanged);
	UVitruvioComponent::OnHierarchyChanged.AddRaw(this, &FVitruvioComponentDetails::OnVitruvioComponentHierarchyChanged);
}

FVitruvioComponentDetails::~FVitruvioComponentDetails()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
	UVitruvioComponent::OnHierarchyChanged.RemoveAll(this);
}

TSharedRef<IDetailCustomization> FVitruvioComponentDetails::MakeInstance()
{
	return MakeShareable(new FVitruvioComponentDetails);
}

void FVitruvioComponentDetails::BuildAttributeEditor(IDetailCategoryBuilder& RootCategory, UVitruvioComponent* VitruvioActor)
{
	if (!VitruvioActor || !VitruvioActor->GetRpk())
	{
		return;
	}

	Generators.Empty();

	IDetailGroup& RootGroup = RootCategory.AddGroup("Attributes", FText::FromString("Attributes"), true, true);
	TMap<FString, IDetailGroup*> GroupCache;
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	for (const auto& AttributeEntry : VitruvioActor->GetAttributes())
	{
		URuleAttribute* Attribute = AttributeEntry.Value;

		IDetailGroup* Group = GetOrCreateGroups(RootGroup, Attribute->Groups, GroupCache);
		
		if (Cast<UStringArrayAttribute>(Attribute) || Cast<UFloatArrayAttribute>(Attribute) || Cast<UBoolArrayAttribute>(Attribute))
		{
			FPropertyRowGeneratorArgs Args;
			const auto Generator = PropertyEditorModule.CreatePropertyRowGenerator(Args);
			TArray<UObject*> Objects;
			Objects.Add(Attribute);
			Generator->SetObjects(Objects);
			Generator->OnFinishedChangingProperties().AddLambda([this, VitruvioActor, Attribute](const FPropertyChangedEvent Event) {
				IDetailLayoutBuilder* DetailBuilder = CachedDetailBuilder.Pin().Get();
				if (DetailBuilder)
				{
					DetailBuilder->ForceRefreshDetails();
				}

				VitruvioActor->EvaluateRuleAttributes(VitruvioActor->GenerateAutomatically);
				Attribute->UserSet = true;
			});
			const TArray<TSharedRef<IDetailTreeNode>> DetailTreeNodes = Generator->GetRootTreeNodes();
			
			AddArrayWidget(DetailTreeNodes, *Group, Attribute, VitruvioActor);

			Generators.Add(Generator);
		}
		else
		{
			FDetailWidgetRow& Row = Group->AddWidgetRow();
			
			Row.FilterTextString = FText::FromString(Attribute->DisplayName);
			Row.NameContent()[CreateNameWidget(Attribute).ToSharedRef()];

			if (UFloatAttribute* FloatAttribute = Cast<UFloatAttribute>(Attribute))
			{
				if (FloatAttribute->GetEnumAnnotation() && FloatAttribute->GetEnumAnnotation()->Values.Num() > 0)
				{
					Row.ValueContent()[CreateScalarEnumWidget<UFloatAttribute, double, UFloatEnumAnnotation>(
										   FloatAttribute, FloatAttribute->GetEnumAnnotation(), VitruvioActor)
										   .ToSharedRef()];
				}
				else
				{
					auto Setter = [VitruvioActor, FloatAttribute](double Value, ETextCommit::Type Type) {
						UpdateAttributeValue(VitruvioActor, FloatAttribute, Value);
					};

					auto Getter = [&FloatAttribute](){
						return FloatAttribute->Value;	
					};
					
					Row.ValueContent()[CreateNumericInputWidget(FloatAttribute, Setter, Getter).ToSharedRef()];
				}
			}
			else if (UStringAttribute* StringAttribute = Cast<UStringAttribute>(Attribute))
			{
				if (StringAttribute->GetEnumAnnotation())
				{
					Row.ValueContent()[CreateScalarEnumWidget<UStringAttribute, FString, UStringEnumAnnotation>(
										   StringAttribute, StringAttribute->GetEnumAnnotation(), VitruvioActor)
										   .ToSharedRef()];
				}
				else if (StringAttribute->GetColorAnnotation())
				{
					auto ColorSetter = [StringAttribute, VitruvioActor](FLinearColor NewColor) {
						UpdateAttributeValue(VitruvioActor, StringAttribute, TEXT("#") + NewColor.ToFColor(true).ToHex());
					};
					auto ColorGetter = [StringAttribute]()
					{
						return FLinearColor(FColor::FromHex(StringAttribute->Value));
					};
					
					Row.ValueContent()[CreateColorInputWidget(ColorSetter, ColorGetter).ToSharedRef()];
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
	TSharedPtr<IPropertyHandle> GenerateAutomaticallyProperty =
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UVitruvioComponent, GenerateAutomatically), UVitruvioComponent::StaticClass());
	GenerateAutomaticallyProperty->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateSP(this, &FVitruvioComponentDetails::OnGenerateAutomaticallyChanged));

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

void FVitruvioComponentDetails::OnGenerateAutomaticallyChanged()
{
	IDetailLayoutBuilder* DetailBuilder = CachedDetailBuilder.Pin().Get();
	if (DetailBuilder)
	{
		DetailBuilder->ForceRefreshDetails();
	}
}

void FVitruvioComponentDetails::OnAttributesChanged(UObject* Object, struct FPropertyChangedEvent& Event)
{
	if (!Event.Property || Event.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	const FName PropertyName = Event.Property->GetFName();
	if (PropertyName == FName(TEXT("Attributes")))
	{
		const auto DetailBuilder = CachedDetailBuilder.Pin().Get();

		if (!DetailBuilder)
		{
			return;
		}

		TArray<TWeakObjectPtr<UObject>> Objects;
		DetailBuilder->GetObjectsBeingCustomized(Objects);

		if (Objects.Num() == 1)
		{
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
}

void FVitruvioComponentDetails::OnVitruvioComponentHierarchyChanged(UVitruvioComponent* Component)
{
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");

	const auto DetailBuilder = CachedDetailBuilder.Pin().Get();

	if (!DetailBuilder)
	{
		return;
	}

	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder->GetObjectsBeingCustomized(Objects);

	if (Objects.Num() == 1)
	{
		UObject* ObjectModified = Objects[0].Get();
		AActor* Owner = nullptr;
		if (Component)
		{
			Owner = Component->GetOwner();
		}

		if (ObjectModified == Component || ObjectModified == Owner)
		{
			LevelEditor.OnComponentsEdited().Broadcast();
		}
	}
}