/* Copyright 2023 Esri
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

#include "VitruvioComponent.h"

#include "AttributeConversion.h"
#include "GeneratedModelHISMComponent.h"
#include "GeneratedModelStaticMeshComponent.h"
#include "MaterialConversion.h"
#include "UnrealCallbacks.h"
#include "VitruvioModule.h"
#include "VitruvioTypes.h"

#include "Algo/Transform.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMeshActor.h"
#include "ObjectEditorUtils.h"
#include "PRTUtils.h"
#include "PhysicsEngine/BodySetup.h"
#include "PolygonWindings.h"
#include "StaticMeshAttributes.h"

#if WITH_EDITOR
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SSpinBox.h"
#endif

namespace
{

bool ToBool(const FString& Value)
{
	if (Value.ToLower() == "true")
	{
		return true;
	}
	if (Value.IsNumeric())
	{
		const int32 NumericValue = StaticCast<int32>(FCString::Atof(*Value));
		return NumericValue != 0;
	}
	return false;
}

template <typename T>
void SetInitialShape(UVitruvioComponent* Component, bool bGenerateModel, UGenerateCompletedCallbackProxy* CallbackProxy, T&& InitialShapeInitializer)
{
	if (Component->InitialShape)
	{
		Component->DestroyInitialShapeComponent();
		Component->InitialShape->Rename(nullptr, GetTransientPackage()); // Remove from Owner
	}

	UInitialShape* NewInitialShape = InitialShapeInitializer();

	Component->InitialShape = NewInitialShape;

	Component->RemoveGeneratedMeshes();
	Component->EvaluateRuleAttributes(bGenerateModel, CallbackProxy);
}

FVector3f GetCentroid(const TArray<FVector3f>& Vertices)
{
	FVector3f Centroid = FVector3f::ZeroVector;
	for (const FVector3f& Vertex : Vertices)
	{
		Centroid += Vertex;
	}
	Centroid /= FMath::Max(1, Vertices.Num());

	return Centroid;
}

template <typename A, typename T>
bool GetAttribute(const TMap<FString, URuleAttribute*>& Attributes, const FString& Name, T& OutValue)
{
	URuleAttribute const* const* FoundAttribute = Attributes.Find(Name);
	if (!FoundAttribute)
	{
		OutValue = {};
		return false;
	}

	URuleAttribute const* Attribute = *FoundAttribute;
	A const* TAttribute = Cast<A>(Attribute);
	if (!TAttribute)
	{
		OutValue = {};
		return false;
	}

	if constexpr (TIsTArray<T>::Value)
	{
		OutValue = TAttribute->Values;
	}
	else
	{
		OutValue = TAttribute->Value;
	}

	return true;
}

template <typename A, typename T>
void SetAttribute(UVitruvioComponent* VitruvioComponent, const TMap<FString, URuleAttribute*>& Attributes, const FString& Name, const T& Value,
				  bool bEvaluateAttributes, bool bGenerateModel, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	URuleAttribute* const* FoundAttribute = Attributes.Find(Name);
	if (!FoundAttribute)
	{
		return;
	}

	URuleAttribute* Attribute = FoundAttribute ? *FoundAttribute : nullptr;
	A* TAttribute = Cast<A>(Attribute);
	if (!TAttribute)
	{
		return;
	}

	if constexpr (TIsTArray<T>::Value)
	{
		TAttribute->Values = Value;
	}
	else
	{
		TAttribute->Value = Value;
	}

	TAttribute->bUserSet = true;

	if (bEvaluateAttributes || bGenerateModel)
	{
		VitruvioComponent->EvaluateRuleAttributes(bGenerateModel, CallbackProxy);
	}
}

template <typename A, typename T>
void EvaluateAndSetAttribute(UVitruvioComponent* VitruvioComponent, TMap<FString, URuleAttribute*>& Attributes, const FString& Name, const T& Value,
							 bool bGenerateModel, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	VitruvioComponent->Initialize();

	if (!VitruvioComponent->GetAttributesReady())
	{
		UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
		Proxy->OnAttributesEvaluated.AddLambda(
			[=, &Attributes]() { SetAttribute<A, T>(VitruvioComponent, Attributes, Name, Value, true, bGenerateModel, CallbackProxy); });
		Proxy->RegisterWithGameInstance(VitruvioComponent);
		VitruvioComponent->EvaluateRuleAttributes(bGenerateModel, Proxy);
	}
	else
	{
		SetAttribute<A, T>(VitruvioComponent, Attributes, Name, Value, true, bGenerateModel, CallbackProxy);
	}
}

void EvaluateAndSetAttributes(UVitruvioComponent* VitruvioComponent, const TMap<FString, FString>& NewAttributes, bool bGenerateModel,
							  UGenerateCompletedCallbackProxy* CallbackProxy)
{
	VitruvioComponent->Initialize();

	if (!VitruvioComponent->GetAttributesReady())
	{
		UGenerateCompletedCallbackProxy* Proxy = NewObject<UGenerateCompletedCallbackProxy>();
		Proxy->OnAttributesEvaluated.AddLambda([=]() { EvaluateAndSetAttributes(VitruvioComponent, NewAttributes, bGenerateModel, CallbackProxy); });
		Proxy->RegisterWithGameInstance(VitruvioComponent);
		VitruvioComponent->EvaluateRuleAttributes(bGenerateModel, Proxy);
	}
	else
	{
		for (const auto& KeyValues : NewAttributes)
		{
			const FString& Value = KeyValues.Value;
			const FString& Key = KeyValues.Key;

			URuleAttribute* const* AttributeResult = VitruvioComponent->GetAttributes().Find(Key);
			if (!AttributeResult)
			{
				continue;
			}

			URuleAttribute const* Attribute = *AttributeResult;

			if (Cast<UFloatAttribute>(Attribute))
			{
				SetAttribute<UFloatAttribute, double>(VitruvioComponent, VitruvioComponent->GetAttributes(), Key, FCString::Atof(*Value), false,
													  false, nullptr);
			}
			else if (Cast<UBoolAttribute>(Attribute))
			{
				SetAttribute<UBoolAttribute, bool>(VitruvioComponent, VitruvioComponent->GetAttributes(), Key, ToBool(Value), false, false, nullptr);
			}
			else if (Cast<UStringAttribute>(Attribute))
			{
				SetAttribute<UStringAttribute, FString>(VitruvioComponent, VitruvioComponent->GetAttributes(), Key, Value, false, false, nullptr);
			}
			else if (Cast<UArrayAttribute>(Attribute))
			{
				FString ArrayValue = Value;
				if (Value.StartsWith(TEXT("[")) && Value.EndsWith(TEXT("]")))
				{
					ArrayValue = Value.LeftChop(1).RightChop(1);
				}

				TArray<FString> StringValues;
				ArrayValue.ParseIntoArray(StringValues, TEXT(","));

				if (Cast<UFloatArrayAttribute>(Attribute))
				{
					TArray<double> DoubleValues;
					Algo::Transform(StringValues, DoubleValues, [](const auto& In) { return FCString::Atof(*In); });
					SetAttribute<UFloatArrayAttribute, TArray<double>>(VitruvioComponent, VitruvioComponent->GetAttributes(), Key, DoubleValues,
																	   false, false, nullptr);
				}
				else if (Cast<UBoolArrayAttribute>(Attribute))
				{
					TArray<bool> BoolValues;
					Algo::Transform(StringValues, BoolValues, [](const auto& In) { return ToBool(In); });
					SetAttribute<UBoolArrayAttribute, TArray<bool>>(VitruvioComponent, VitruvioComponent->GetAttributes(), Key, BoolValues, false,
																	false, nullptr);
				}
				else
				{
					SetAttribute<UStringArrayAttribute, TArray<FString>>(VitruvioComponent, VitruvioComponent->GetAttributes(), Key, StringValues,
																		 false, false, nullptr);
				}
			}
		}

		VitruvioComponent->EvaluateRuleAttributes(bGenerateModel, CallbackProxy);
	}
}

bool IsOuterOf(UObject* Inner, UObject* Outer)
{
	if (!Outer)
	{
		return false;
	}

	UObject* Current = Inner->GetOuter();

	while (Current != nullptr)
	{
		if (Current == Outer)
		{
			return true;
		}

		Current = Current->GetOuter();
	}

	return false;
}

void InitializeBodySetup(UBodySetup* BodySetup, bool GenerateComplexCollision)
{
	BodySetup->DefaultInstance.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	BodySetup->CollisionTraceFlag =
		GenerateComplexCollision ? ECollisionTraceFlag::CTF_UseComplexAsSimple : ECollisionTraceFlag::CTF_UseSimpleAsComplex;
	BodySetup->bDoubleSidedGeometry = true;
	BodySetup->bMeshCollideAll = true;
	BodySetup->InvalidatePhysicsData();
	BodySetup->CreatePhysicsMeshes();
}

void CreateCollision(UStaticMesh* Mesh, UStaticMeshComponent* StaticMeshComponent, bool ComplexCollision)
{
	if (!Mesh)
	{
		return;
	}

	UBodySetup* BodySetup =
		NewObject<UBodySetup>(StaticMeshComponent, NAME_None, RF_Transient | RF_DuplicateTransient | RF_TextExportTransient | RF_Transactional);
	InitializeBodySetup(BodySetup, ComplexCollision);
	Mesh->SetBodySetup(BodySetup);
	StaticMeshComponent->RecreatePhysicsState();
}

#if WITH_EDITOR
bool IsRelevantObject(UVitruvioComponent* VitruvioComponent, UObject* Object)
{
	if (VitruvioComponent == Object || VitruvioComponent->InitialShape == Object)
	{
		return true;
	}
	AActor* Owner = VitruvioComponent->GetOwner();
	if (!Owner)
	{
		return false;
	}

	const TSet<UActorComponent*>& Components = Owner->GetComponents();
	for (UActorComponent* ChildComponent : Components)
	{
		if (ChildComponent == Object)
		{
			return true;
		}
	}

	return false;
}
#endif

FString UniqueComponentName(const FString& Name, TMap<FString, int32>& UsedNames)
{
	FString CurrentName = Name;
	while (UsedNames.Contains(CurrentName))
	{
		const int32 Count = UsedNames[CurrentName]++;
		CurrentName = Name + FString::FromInt(Count);
	}
	UsedNames.Add(CurrentName, 0);
	return CurrentName;
}

} // namespace

UVitruvioComponent::FOnHierarchyChanged UVitruvioComponent::OnHierarchyChanged;
UVitruvioComponent::FOnAttributesChanged UVitruvioComponent::OnAttributesChanged;

UVitruvioComponent::UVitruvioComponent()
{
	static ConstructorHelpers::FObjectFinder<UMaterial> Opaque(TEXT("Material'/Vitruvio/Materials/M_OpaqueParent.M_OpaqueParent'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Masked(TEXT("Material'/Vitruvio/Materials/M_MaskedParent.M_MaskedParent'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> Translucent(TEXT("Material'/Vitruvio/Materials/M_TranslucentParent.M_TranslucentParent'"));
	OpaqueParent = Opaque.Object;
	MaskedParent = Masked.Object;
	TranslucentParent = Translucent.Object;

	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
}

void UVitruvioComponent::CalculateRandomSeed()
{
	if (!bValidRandomSeed && InitialShape && InitialShape->IsValid())
	{
		const FVector3f Centroid = GetCentroid(InitialShape->GetVertices());
		RandomSeed = GetTypeHash(GetOwner()->GetActorTransform().TransformPosition(FVector(Centroid)));
		bValidRandomSeed = true;
	}
}

bool UVitruvioComponent::HasValidInputData() const
{
	return InitialShape && InitialShape->IsValid() && Rpk;
}

bool UVitruvioComponent::IsReadyToGenerate() const
{
	return HasValidInputData() && bAttributesReady;
}

void UVitruvioComponent::SetRpk(URulePackage* RulePackage, bool bGenerateModel, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	if (this->Rpk == RulePackage)
	{
		return;
	}

	this->Rpk = RulePackage;

	Attributes.Empty();
	bAttributesReady = false;
	bNotifyAttributeChange = true;

	RemoveGeneratedMeshes();
	EvaluateRuleAttributes(bGenerateModel, CallbackProxy);
}

void UVitruvioComponent::SetStringAttribute(const FString& Name, const FString& Value, bool bGenerateModel,
											UGenerateCompletedCallbackProxy* CallbackProxy)
{
	EvaluateAndSetAttribute<UStringAttribute, FString>(this, this->Attributes, Name, Value, bGenerateModel, CallbackProxy);
}

bool UVitruvioComponent::GetStringAttribute(const FString& Name, FString& OutValue) const
{
	return GetAttribute<UStringAttribute, FString>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetStringArrayAttribute(const FString& Name, const TArray<FString>& Values, bool bGenerateModel,
												 UGenerateCompletedCallbackProxy* CallbackProxy)
{
	EvaluateAndSetAttribute<UStringArrayAttribute, TArray<FString>>(this, this->Attributes, Name, Values, bGenerateModel, CallbackProxy);
}

bool UVitruvioComponent::GetStringArrayAttribute(const FString& Name, TArray<FString>& OutValue) const
{
	return GetAttribute<UStringArrayAttribute, TArray<FString>>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetBoolAttribute(const FString& Name, bool Value, bool bGenerateModel, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	EvaluateAndSetAttribute<UBoolAttribute, bool>(this, this->Attributes, Name, Value, bGenerateModel, CallbackProxy);
}

bool UVitruvioComponent::GetBoolAttribute(const FString& Name, bool& OutValue) const
{
	return GetAttribute<UBoolAttribute, bool>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetBoolArrayAttribute(const FString& Name, const TArray<bool>& Values, bool bGenerateModel,
											   UGenerateCompletedCallbackProxy* CallbackProxy)
{
	EvaluateAndSetAttribute<UBoolArrayAttribute, TArray<bool>>(this, this->Attributes, Name, Values, bGenerateModel, CallbackProxy);
}

bool UVitruvioComponent::GetBoolArrayAttribute(const FString& Name, TArray<bool>& OutValue) const
{
	return GetAttribute<UBoolArrayAttribute, TArray<bool>>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetFloatAttribute(const FString& Name, double Value, bool bGenerateModel, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	EvaluateAndSetAttribute<UFloatAttribute, double>(this, this->Attributes, Name, Value, bGenerateModel, CallbackProxy);
}

bool UVitruvioComponent::GetFloatAttribute(const FString& Name, double& OutValue) const
{
	return GetAttribute<UFloatAttribute, double>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetFloatArrayAttribute(const FString& Name, const TArray<double>& Values, bool bGenerateModel,
												UGenerateCompletedCallbackProxy* CallbackProxy)
{
	EvaluateAndSetAttribute<UFloatArrayAttribute, TArray<double>>(this, this->Attributes, Name, Values, bGenerateModel, CallbackProxy);
}

bool UVitruvioComponent::GetFloatArrayAttribute(const FString& Name, TArray<double>& OutValue) const
{
	return GetAttribute<UFloatArrayAttribute, TArray<double>>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetAttributes(const TMap<FString, FString>& NewAttributes, bool bGenerateModel,
									   UGenerateCompletedCallbackProxy* CallbackProxy)
{
	EvaluateAndSetAttributes(this, NewAttributes, bGenerateModel, CallbackProxy);
}

void UVitruvioComponent::SetMeshInitialShape(UStaticMesh* StaticMesh, bool bGenerateModel, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	SetInitialShape(this, bGenerateModel, CallbackProxy, [this, StaticMesh]() {
		UStaticMeshInitialShape* NewInitialShape =
			NewObject<UStaticMeshInitialShape>(GetOwner(), UStaticMeshInitialShape::StaticClass(), NAME_None, RF_Transactional);
		InitialShapeSceneComponent = NewInitialShape->CreateInitialShapeComponent(this, StaticMesh);
		return NewInitialShape;
	});
}

void UVitruvioComponent::SetSplineInitialShape(const TArray<FSplinePoint>& SplinePoints, bool bGenerateModel,
											   UGenerateCompletedCallbackProxy* CallbackProxy)
{
	SetInitialShape(this, bGenerateModel, CallbackProxy, [this, &SplinePoints]() {
		USplineInitialShape* NewInitialShape =
			NewObject<USplineInitialShape>(GetOwner(), USplineInitialShape::StaticClass(), NAME_None, RF_Transactional);
		InitialShapeSceneComponent = NewInitialShape->CreateInitialShapeComponent(this, SplinePoints);
		return NewInitialShape;
	});
}

const TMap<FString, URuleAttribute*>& UVitruvioComponent::GetAttributes() const
{
	return Attributes;
}

URulePackage* UVitruvioComponent::GetRpk() const
{
	return Rpk;
}

const TMap<FString, FReport>& UVitruvioComponent::GetReports() const
{
	return Reports;
}

void UVitruvioComponent::SetInitialShapeVisible(bool bVisible)
{
	InitialShapeSceneComponent->SetVisibility(bVisible, false);
	InitialShapeSceneComponent->SetHiddenInGame(!bVisible);
}

void UVitruvioComponent::SetRandomSeed(int32 NewRandomSeed, bool bGenerateModel, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	RandomSeed = NewRandomSeed;
	bValidRandomSeed = true;

	EvaluateRuleAttributes(bGenerateModel, CallbackProxy);
}

void UVitruvioComponent::LoadInitialShape()
{
	if (InitialShape)
	{
		InitialShapeSceneComponent = InitialShape->CreateInitialShapeComponent(this);
		return;
	}

	// Detect initial initial shape type (eg if there has already been a StaticMeshComponent assigned to the actor)
	check(GetInitialShapesClasses().Num() > 0);
	for (const auto& InitialShapeClasses : GetInitialShapesClasses())
	{
		UInitialShape* DefaultInitialShape = Cast<UInitialShape>(InitialShapeClasses->GetDefaultObject());
		if (DefaultInitialShape && DefaultInitialShape->CanConstructFrom(this->GetOwner()))
		{
			InitialShape = NewObject<UInitialShape>(GetOwner(), DefaultInitialShape->GetClass(), NAME_None, RF_Transactional);
			break;
		}
	}

	if (!InitialShape)
	{
		InitialShape = NewObject<UInitialShape>(GetOwner(), GetInitialShapesClasses()[0], NAME_None, RF_Transactional);
	}

	InitialShape->Initialize();
	InitialShapeSceneComponent = InitialShape->CreateInitialShapeComponent(this);
	InitialShape->UpdatePolygon(this);
}

void UVitruvioComponent::Initialize()
{
	if (bInitialized)
	{
		return;
	}

	bInitialized = true;

	// During cooking we don't need to do initialize anything as we don't wont to generate during the cooking process
	if (GIsCookerLoadingPackage)
	{
		return;
	}

#if WITH_EDITOR
	if (!PropertyChangeDelegate.IsValid())
	{
		PropertyChangeDelegate = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UVitruvioComponent::OnPropertyChanged);
	}
#endif

	LoadInitialShape();

	OnHierarchyChanged.Broadcast(this);

	CalculateRandomSeed();
}

void UVitruvioComponent::ProcessGenerateQueue()
{
	if (!GenerateQueue.IsEmpty())
	{
		// Get from queue and build meshes
		FGenerateQueueItem Result;
		GenerateQueue.Dequeue(Result);

		FConvertedGenerateResult ConvertedResult =
			BuildResult(Result.GenerateResultDescription, VitruvioModule::Get().GetMaterialCache(), VitruvioModule::Get().GetTextureCache());

		Reports = ConvertedResult.Reports;

		QUICK_SCOPE_CYCLE_COUNTER(STAT_VitruvioActor_CreateModelActors);

		UGeneratedModelStaticMeshComponent* VitruvioModelComponent = nullptr;

		TArray<USceneComponent*> InitialShapeChildComponents;
		InitialShapeSceneComponent->GetChildrenComponents(false, InitialShapeChildComponents);
		for (USceneComponent* Component : InitialShapeChildComponents)
		{
			if (Component->IsA(UGeneratedModelStaticMeshComponent::StaticClass()))
			{
				VitruvioModelComponent = Cast<UGeneratedModelStaticMeshComponent>(Component);

				VitruvioModelComponent->SetStaticMesh(nullptr);

				// Cleanup old hierarchical instances
				TArray<USceneComponent*> InstanceComponents;
				VitruvioModelComponent->GetChildrenComponents(true, InstanceComponents);
				for (USceneComponent* InstanceComponent : InstanceComponents)
				{
					InstanceComponent->DestroyComponent(true);
				}

				break;
			}
		}

		if (!VitruvioModelComponent)
		{
			VitruvioModelComponent = NewObject<UGeneratedModelStaticMeshComponent>(InitialShapeSceneComponent, FName(TEXT("GeneratedModel")),
																				   RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
			VitruvioModelComponent->CreationMethod = EComponentCreationMethod::Instance;
			InitialShapeSceneComponent->GetOwner()->AddOwnedComponent(VitruvioModelComponent);
			VitruvioModelComponent->AttachToComponent(InitialShapeSceneComponent, FAttachmentTransformRules::KeepRelativeTransform);
			VitruvioModelComponent->OnComponentCreated();
			VitruvioModelComponent->RegisterComponent();
		}

		if (ConvertedResult.ShapeMesh)
		{
			VitruvioModelComponent->SetStaticMesh(ConvertedResult.ShapeMesh->GetStaticMesh());
			VitruvioModelComponent->SetCollisionData(ConvertedResult.ShapeMesh->GetCollisionData());
			CreateCollision(ConvertedResult.ShapeMesh->GetStaticMesh(), VitruvioModelComponent, GenerateCollision);
		}
		else
		{
			VitruvioModelComponent->SetStaticMesh(nullptr);
			VitruvioModelComponent->SetCollisionData({});
		}

		TMap<FString, int32> NameMap;
		for (const FInstance& Instance : ConvertedResult.Instances)
		{
			FString UniqueName = UniqueComponentName(Instance.Name, NameMap);
			auto InstancedComponent = NewObject<UGeneratedModelHISMComponent>(VitruvioModelComponent, FName(UniqueName),
																			  RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
			const TArray<FTransform>& Transforms = Instance.Transforms;
			InstancedComponent->SetStaticMesh(Instance.InstanceMesh->GetStaticMesh());
			InstancedComponent->SetCollisionData(Instance.InstanceMesh->GetCollisionData());

			// Add all instance transforms
			for (const FTransform& Transform : Transforms)
			{
				InstancedComponent->AddInstance(Transform);
			}

			// Apply override materials
			for (int32 MaterialIndex = 0; MaterialIndex < Instance.OverrideMaterials.Num(); ++MaterialIndex)
			{
				InstancedComponent->SetMaterial(MaterialIndex, Instance.OverrideMaterials[MaterialIndex]);
			}

			// Instanced component collision
			CreateCollision(Instance.InstanceMesh->GetStaticMesh(), InstancedComponent, GenerateCollision);

			// Attach and register instance component
			InstancedComponent->AttachToComponent(VitruvioModelComponent, FAttachmentTransformRules::KeepRelativeTransform);
			InstancedComponent->CreationMethod = EComponentCreationMethod::Instance;
			InitialShapeSceneComponent->GetOwner()->AddOwnedComponent(InstancedComponent);
			InstancedComponent->OnComponentCreated();
			InstancedComponent->RegisterComponent();
		}

		OnHierarchyChanged.Broadcast(this);

		HasGeneratedMesh = true;

		SetInitialShapeVisible(!HideAfterGeneration);

		if (Result.CallbackProxy)
		{
			Result.CallbackProxy->OnGenerateCompletedBlueprint.Broadcast();
			Result.CallbackProxy->OnGenerateCompleted.Broadcast();
			Result.CallbackProxy->SetReadyToDestroy();
		}
		OnGenerateCompleted.Broadcast();
	}
}

void UVitruvioComponent::ProcessAttributesEvaluationQueue()
{
	if (!AttributesEvaluationQueue.IsEmpty())
	{
		FAttributesEvaluationQueueItem AttributesEvaluation;
		AttributesEvaluationQueue.Dequeue(AttributesEvaluation);

		AttributesEvaluation.AttributeMap->UpdateUnrealAttributeMap(Attributes, this);

		bAttributesReady = true;
		bNotifyAttributeChange = true;

		if (AttributesEvaluation.CallbackProxy)
		{
			AttributesEvaluation.CallbackProxy->OnAttributesEvaluatedBlueprint.Broadcast();
			AttributesEvaluation.CallbackProxy->OnAttributesEvaluated.Broadcast();
		}
		OnAttributesEvaluated.Broadcast();

		if (AttributesEvaluation.bForceRegenerate)
		{
			Generate(AttributesEvaluation.CallbackProxy);
		}
		else if (AttributesEvaluation.CallbackProxy)
		{
			AttributesEvaluation.CallbackProxy->SetReadyToDestroy();
		}
	}
}

void UVitruvioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (!bInitialized)
	{
		Initialize();
		if (!EvalAttributesInvalidationToken.IsValid())
		{
			EvaluateRuleAttributes(GenerateAutomatically);
		}
	}

	ProcessGenerateQueue();
	ProcessAttributesEvaluationQueue();

	if (bNotifyAttributeChange)
	{
		NotifyAttributesChanged();
		bNotifyAttributeChange = false;
	}
}

void UVitruvioComponent::NotifyAttributesChanged()
{
#if WITH_EDITOR
	// Notify possible listeners (eg. Details panel) about changes to the Attributes
	FPropertyChangedEvent PropertyEvent(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UVitruvioComponent, Attributes)));
	OnAttributesChanged.Broadcast(this, PropertyEvent);
#endif
}

void UVitruvioComponent::RemoveGeneratedMeshes()
{
	if (!InitialShape || !InitialShapeSceneComponent)
	{
		return;
	}

	TArray<USceneComponent*> Children;
	InitialShapeSceneComponent->GetChildrenComponents(true, Children);
	for (USceneComponent* Child : Children)
	{
		Child->DestroyComponent();
	}

	HasGeneratedMesh = false;
	SetInitialShapeVisible(true);
}

bool UVitruvioComponent::GetAttributesReady()
{
	return bAttributesReady;
}

FConvertedGenerateResult UVitruvioComponent::BuildResult(FGenerateResultDescription& GenerateResult,
														 TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
														 TMap<FString, Vitruvio::FTextureData>& TextureCache)
{
	// build all meshes
	for (auto& IdAndMesh : GenerateResult.Meshes)
	{
		FString Name = GenerateResult.Names[IdAndMesh.Key];
		IdAndMesh.Value->Build(Name, MaterialCache, TextureCache, OpaqueParent, MaskedParent, TranslucentParent);
	}

	// convert instances
	TArray<FInstance> Instances;
	for (const auto& Instance : GenerateResult.Instances)
	{
		auto VitruvioMesh = GenerateResult.Meshes[Instance.Key.PrototypeId];
		FString MeshName = GenerateResult.Names[Instance.Key.PrototypeId];
		TArray<UMaterialInstanceDynamic*> OverrideMaterials;
		for (size_t MaterialIndex = 0; MaterialIndex < Instance.Key.MaterialOverrides.Num(); ++MaterialIndex)
		{
			const Vitruvio::FMaterialAttributeContainer& MaterialContainer = Instance.Key.MaterialOverrides[MaterialIndex];
			FName MaterialName = FName(MaterialContainer.Name);
			OverrideMaterials.Add(CacheMaterial(OpaqueParent, MaskedParent, TranslucentParent, TextureCache, MaterialCache, MaterialContainer,
												VitruvioMesh->GetStaticMesh()));
		}

		Instances.Add({MeshName, VitruvioMesh, OverrideMaterials, Instance.Value});
	}

	TSharedPtr<FVitruvioMesh> ShapeMesh = GenerateResult.Meshes.Contains(UnrealCallbacks::NO_PROTOTYPE_INDEX)
											  ? GenerateResult.Meshes[UnrealCallbacks::NO_PROTOTYPE_INDEX]
											  : TSharedPtr<FVitruvioMesh>{};

	return {ShapeMesh, Instances, GenerateResult.Reports};
}

void UVitruvioComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (GenerateToken)
	{
		GenerateToken->Invalidate();
	}

	if (EvalAttributesInvalidationToken)
	{
		EvalAttributesInvalidationToken->Invalidate();
	}

#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangeDelegate);
	PropertyChangeDelegate.Reset();
#endif
}

void UVitruvioComponent::Generate(UGenerateCompletedCallbackProxy* CallbackProxy)
{
	Initialize();

	// If either the RPK, initial shape or attributes are not ready we can not generate
	if (!HasValidInputData())
	{
		RemoveGeneratedMeshes();
		return;
	}

	// Since we can not abort an ongoing generate call from PRT, we invalidate the result and regenerate after the current generate call has
	// completed.
	if (GenerateToken)
	{
		GenerateToken->Invalidate();
		GenerateToken.Reset();
	}

	if (InitialShape)
	{
		FGenerateResult GenerateResult =
			VitruvioModule::Get().GenerateAsync(InitialShape->GetPolygon(), Rpk, Vitruvio::CreateAttributeMap(Attributes), RandomSeed);

		GenerateToken = GenerateResult.Token;

		// clang-format off
		GenerateResult.Result.Next([this, CallbackProxy](const FGenerateResult::ResultType& Result)
		{
			FScopeLock Lock(&Result.Token->Lock);

			if (Result.Token->IsInvalid()) {
				return;
			}

			GenerateToken.Reset();
			GenerateQueue.Enqueue({Result.Value, CallbackProxy});
		});
		// clang-format on
	}
}

#if WITH_EDITOR

void UVitruvioComponent::PreEditUndo()
{
	Super::PreEditUndo();

	DestroyInitialShapeComponent();
}

void UVitruvioComponent::PostUndoRedo()
{
	DestroyInitialShapeComponent();
	InitializeInitialShapeComponent();

	if (!PropertyChangeDelegate.IsValid())
	{
		PropertyChangeDelegate = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UVitruvioComponent::OnPropertyChanged);
	}

	Generate();
}

void UVitruvioComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	OnPropertyChanged(this, PropertyChangedEvent);
}

void UVitruvioComponent::OnPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (Object != this && !IsOuterOf(Object, this->GetOwner()))
	{
		return;
	}
	bool bComponentPropertyChanged = false;

	if (PropertyChangedEvent.Property != nullptr)
	{
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, Rpk))
		{
			Attributes.Empty();
			bAttributesReady = false;
			bComponentPropertyChanged = true;
			bNotifyAttributeChange = true;
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, RandomSeed))
		{
			bValidRandomSeed = true;
			bComponentPropertyChanged = true;
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, GenerateCollision))
		{
			bComponentPropertyChanged = true;
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, HideAfterGeneration))
		{
			SetInitialShapeVisible(!(HideAfterGeneration && HasGeneratedMesh));
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, GenerateAutomatically))
		{
			bComponentPropertyChanged = true;
		}
	}

	// If an object was changed via an undo command, the PropertyChangedEvent.Property is null
	// Therefore, we can only check the ObjectType and check if the Property is null (=likely undo event)
	// This is suboptimal, since it can also happen during undo commands on irrelevant properties of that object. Might be improved later...
	const bool bIsSplinePropertyUndo = Object->IsA(USplineComponent::StaticClass()) && PropertyChangedEvent.Property == nullptr;
	const bool bIsAttributeUndo = Object->IsA(URuleAttribute::StaticClass()) && PropertyChangedEvent.Property == nullptr;

	const bool bRelevantProperty =
		InitialShape != nullptr && (bIsSplinePropertyUndo || InitialShape->IsRelevantProperty(Object, PropertyChangedEvent));
	const bool bRecreateInitialShape = IsRelevantObject(this, Object) && bRelevantProperty;

	// If a property has changed which is used by the initial shape we have to update it's polygon
	if (bRecreateInitialShape)
	{
		Modify();
		InitialShape->UpdatePolygon(this);

		CalculateRandomSeed();
	}

	if (bAttributesReady && GenerateAutomatically && (bRecreateInitialShape || bComponentPropertyChanged))
	{
		Generate();
	}

	if (!HasValidInputData())
	{
		RemoveGeneratedMeshes();
	}

	if (HasValidInputData() && (!bAttributesReady || bIsAttributeUndo))
	{
		EvaluateRuleAttributes(true);
	}
}

void UVitruvioComponent::SetInitialShapeType(const TSubclassOf<UInitialShape>& Type)
{
	RemoveGeneratedMeshes();

	UInitialShape* NewInitialShape = NewObject<UInitialShape>(GetOwner(), Type, NAME_None, RF_Transactional);

	if (InitialShape)
	{
		if (InitialShape->GetClass() == Type)
		{
			return;
		}

		NewInitialShape->SetPolygon(InitialShape->GetPolygon());

		InitialShape->Rename(nullptr, GetTransientPackage()); // Remove from Owner
		InitialShape = NewInitialShape;

		DestroyInitialShapeComponent();
		InitializeInitialShapeComponent();
	}
	else
	{
		InitialShape = NewInitialShape;
		InitializeInitialShapeComponent();
	}
}

#endif // WITH_EDITOR

void UVitruvioComponent::EvaluateRuleAttributes(bool ForceRegenerate, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	Initialize();

	// If we don't have valid input data (initial shape and rpk) we can not evaluate the rule attribtues
	if (!HasValidInputData())
	{
		return;
	}

	// Since we can not abort an ongoing generate call from PRT, we invalidate the result and evaluate the attributes again after the current request
	// has completed.
	if (EvalAttributesInvalidationToken)
	{
		EvalAttributesInvalidationToken->Invalidate();
		EvalAttributesInvalidationToken.Reset();
	}

	bAttributesReady = false;

	FAttributeMapResult AttributesResult =
		VitruvioModule::Get().EvaluateRuleAttributesAsync(InitialShape->GetPolygon(), Rpk, Vitruvio::CreateAttributeMap(Attributes), RandomSeed);

	EvalAttributesInvalidationToken = AttributesResult.Token;

	AttributesResult.Result.Next([this, CallbackProxy, ForceRegenerate](const FAttributeMapResult::ResultType& Result) {
		FScopeLock Lock(&Result.Token->Lock);

		if (Result.Token->IsInvalid())
		{
			return;
		}

		EvalAttributesInvalidationToken.Reset();
		AttributesEvaluationQueue.Enqueue({Result.Value, ForceRegenerate, CallbackProxy});
	});
}

void UVitruvioComponent::InitializeInitialShapeComponent()
{
	if (InitialShapeSceneComponent)
	{
		return;
	}

	InitialShapeSceneComponent = InitialShape->CreateInitialShapeComponent(this);
	InitialShape->UpdateSceneComponent(this);
}

void UVitruvioComponent::DestroyInitialShapeComponent()
{
	if (!InitialShapeSceneComponent)
	{
		return;
	}

	// Similarly to Unreal Ed component deletion. See ComponentEditorUtils#DeleteComponents
#if WITH_EDITOR
	Modify();
#endif

	// Note that promote to children of DestroyComponent only checks for attached children not actual child components
	// therefore we have to destroy them manually here
	TArray<USceneComponent*> Children;
	InitialShapeSceneComponent->GetChildrenComponents(true, Children);
	for (USceneComponent* Child : Children)
	{
		Child->DestroyComponent(true);
	}

	AActor* Owner = InitialShapeSceneComponent->GetOwner();

	InitialShapeSceneComponent->DestroyComponent();
#if WITH_EDITOR
	Owner->RerunConstructionScripts();
#endif

	InitialShapeSceneComponent = nullptr;
}

TArray<TSubclassOf<UInitialShape>> UVitruvioComponent::GetInitialShapesClasses()
{
	return {UStaticMeshInitialShape::StaticClass(), USplineInitialShape::StaticClass()};
}
