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

#include "VitruvioComponent.h"

#include "AttributeConversion.h"
#include "GeneratedModelHISMComponent.h"
#include "GeneratedModelInstanceComponent.h"
#include "GeneratedModelStaticMeshComponent.h"
#include "MaterialConversion.h"
#include "UnrealCallbacks.h"
#include "VitruvioModule.h"
#include "VitruvioTypes.h"

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
void SetAttribute(UVitruvioComponent* VitruvioComponent, TMap<FString, URuleAttribute*>& Attributes, const FString& Name, const T& Value,
				  bool bAddIfNonExisting, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	URuleAttribute** FoundAttribute = Attributes.Find(Name);
	if (!FoundAttribute && !bAddIfNonExisting)
	{
		return;
	}

	URuleAttribute* Attribute = FoundAttribute ? *FoundAttribute : nullptr;
	A* TAttribute = Cast<A>(Attribute);
	if (!TAttribute)
	{
		if (!bAddIfNonExisting)
		{
			return;
		}

		// Create new attribute if it does not exist and bAddIfNonExisting is true
		A* RuleAttribute = NewObject<A>(VitruvioComponent->GetOuter());
		RuleAttribute->Name = Name;
		RuleAttribute->DisplayName = WCHAR_TO_TCHAR(prtu::removeImport(prtu::removeStyle(*Name)).c_str());
		RuleAttribute->ImportPath = WCHAR_TO_TCHAR(prtu::getFullImportPath(*Name).c_str());

		Attributes.Add(Name, RuleAttribute);

		TAttribute = RuleAttribute;
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
	VitruvioComponent->EvaluateRuleAttributes(VitruvioComponent->GenerateAutomatically, CallbackProxy);
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

	UBodySetup* BodySetup = NewObject<UBodySetup>(StaticMeshComponent, NAME_None, RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
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
		CurrentName = Name + "_" + FString::FromInt(Count);
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

void UVitruvioComponent::SetRpk(URulePackage* RulePackage, UGenerateCompletedCallbackProxy* CallbackProxy)
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
	EvaluateRuleAttributes(GenerateAutomatically, CallbackProxy);
}

void UVitruvioComponent::SetStringAttribute(const FString& Name, const FString& Value, bool bAddIfNonExisting,
											UGenerateCompletedCallbackProxy* CallbackProxy)
{
	SetAttribute<UStringAttribute, FString>(this, this->Attributes, Name, Value, bAddIfNonExisting, CallbackProxy);
}

bool UVitruvioComponent::GetStringAttribute(const FString& Name, FString& OutValue) const
{
	return GetAttribute<UStringAttribute, FString>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetStringArrayAttribute(const FString& Name, const TArray<FString>& Values, bool bAddIfNonExisting,
												 UGenerateCompletedCallbackProxy* CallbackProxy)
{
	SetAttribute<UStringArrayAttribute, TArray<FString>>(this, this->Attributes, Name, Values, bAddIfNonExisting, CallbackProxy);
}

bool UVitruvioComponent::GetStringArrayAttribute(const FString& Name, TArray<FString>& OutValue) const
{
	return GetAttribute<UStringArrayAttribute, TArray<FString>>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetBoolAttribute(const FString& Name, bool Value, bool bAddIfNonExisting, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	SetAttribute<UBoolAttribute, bool>(this, this->Attributes, Name, Value, bAddIfNonExisting, CallbackProxy);
}

bool UVitruvioComponent::GetBoolAttribute(const FString& Name, bool& OutValue) const
{
	return GetAttribute<UBoolAttribute, bool>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetBoolArrayAttribute(const FString& Name, const TArray<bool>& Values, bool bAddIfNonExisting,
											   UGenerateCompletedCallbackProxy* CallbackProxy)
{
	SetAttribute<UBoolArrayAttribute, TArray<bool>>(this, this->Attributes, Name, Values, bAddIfNonExisting, CallbackProxy);
}

bool UVitruvioComponent::GetBoolArrayAttribute(const FString& Name, TArray<bool>& OutValue) const
{
	return GetAttribute<UBoolArrayAttribute, TArray<bool>>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetFloatAttribute(const FString& Name, double Value, bool bAddIfNonExisting, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	SetAttribute<UFloatAttribute, double>(this, this->Attributes, Name, Value, bAddIfNonExisting, CallbackProxy);
}

bool UVitruvioComponent::GetFloatAttribute(const FString& Name, double& OutValue) const
{
	return GetAttribute<UFloatAttribute, double>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetFloatArrayAttribute(const FString& Name, const TArray<double>& Values, bool bAddIfNonExisting,
												UGenerateCompletedCallbackProxy* CallbackProxy)
{
	SetAttribute<UFloatArrayAttribute, TArray<double>>(this, this->Attributes, Name, Values, bAddIfNonExisting, CallbackProxy);
}

bool UVitruvioComponent::GetFloatArrayAttribute(const FString& Name, TArray<double>& OutValue) const
{
	return GetAttribute<UFloatArrayAttribute, TArray<double>>(this->Attributes, Name, OutValue);
}

void UVitruvioComponent::SetAttributes(const TMap<FString, FString>& NewAttributes, bool bAddIfNonExisting,
									   UGenerateCompletedCallbackProxy* CallbackProxy)
{
	const bool bOldGenerateAutomatically = GenerateAutomatically;
	GenerateAutomatically = false;

	for (const auto& KeyValues : NewAttributes)
	{
		const FString& Value = KeyValues.Value;
		const FString& Key = KeyValues.Key;

		if (FCString::IsNumeric(*Value))
		{
			SetFloatAttribute(Key, FCString::Atof(*Value), bAddIfNonExisting, nullptr);
		}
		else if (Value == "true" || Value == "false")
		{
			SetBoolAttribute(Key, Value == "true", bAddIfNonExisting, nullptr);
		}
		else
		{
			SetStringAttribute(Key, Value, bAddIfNonExisting, nullptr);
		}
	}

	GenerateAutomatically = bOldGenerateAutomatically;
	if (HasValidInputData())
	{
		EvaluateRuleAttributes(GenerateAutomatically, CallbackProxy);
	}
}

void UVitruvioComponent::SetMeshInitialShape(UStaticMesh* StaticMesh, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	if (InitialShape)
	{
		InitialShape->Uninitialize();
		InitialShape->Rename(nullptr, GetTransientPackage()); // Remove from Owner
	}

	UStaticMeshInitialShape* NewInitialShape =
		NewObject<UStaticMeshInitialShape>(GetOwner(), UStaticMeshInitialShape::StaticClass(), NAME_None, RF_Transient | RF_TextExportTransient);
	NewInitialShape->Initialize(this, StaticMesh);

	InitialShape = NewInitialShape;

	RemoveGeneratedMeshes();
	EvaluateRuleAttributes(GenerateAutomatically, CallbackProxy);
}

void UVitruvioComponent::SetSplineInitialShape(const TArray<FSplinePoint>& SplinePoints, UGenerateCompletedCallbackProxy* CallbackProxy)
{
	if (InitialShape)
	{
		InitialShape->Uninitialize();
		InitialShape->Rename(nullptr, GetTransientPackage()); // Remove from Owner
	}

	USplineInitialShape* NewInitialShape =
		NewObject<USplineInitialShape>(GetOwner(), USplineInitialShape::StaticClass(), NAME_None, RF_Transient | RF_TextExportTransient);
	NewInitialShape->Initialize(this, SplinePoints);

	InitialShape = NewInitialShape;

	RemoveGeneratedMeshes();
	EvaluateRuleAttributes(GenerateAutomatically, CallbackProxy);
}

void UVitruvioComponent::RemoveInstanceComponent(UGeneratedModelInstanceComponent* GeneratedModelInstanceComponent)
{
	const USceneComponent* InitialShapeComponent = InitialShape->GetComponent();
	TArray<USceneComponent*> InitialShapeChildComponents;
	InitialShapeComponent->GetChildrenComponents(false, InitialShapeChildComponents);
	for (USceneComponent* Component : InitialShapeChildComponents)
	{
		if (Component->IsA(UGeneratedModelStaticMeshComponent::StaticClass()))
		{
			UGeneratedModelStaticMeshComponent* VitruvioModelComponent = Cast<UGeneratedModelStaticMeshComponent>(Component);

			// Cleanup old hierarchical instances
			TArray<USceneComponent*> InstanceComponents;
			VitruvioModelComponent->GetChildrenComponents(true, InstanceComponents);
			for (USceneComponent* InstanceComponent : InstanceComponents)
			{
				if (InstanceComponent == GeneratedModelInstanceComponent)
				{
					InstanceComponent->Rename(nullptr, GetTransientPackage()); // Remove from Owner
					InstanceComponent->DestroyComponent(false);

					return;
				}
			}

			return;
		}
	}
}

const TMap<FString, URuleAttribute*>& UVitruvioComponent::GetAttributes() const
{
	return Attributes;
}

URulePackage* UVitruvioComponent::GetRpk() const
{
	return Rpk;
}

void UVitruvioComponent::SetRandomSeed(int32 NewRandomSeed)
{
	RandomSeed = NewRandomSeed;
	bValidRandomSeed = true;

	if (GenerateAutomatically && IsReadyToGenerate())
	{
		Generate();
	}
}

void UVitruvioComponent::LoadInitialShape()
{
	// Do nothing if an initial shape has already been loaded
	if (InitialShape)
	{
		return;
	}

	// Detect initial initial shape type (eg if there has already been a StaticMeshComponent assigned to the actor)
	check(GetInitialShapesClasses().Num() > 0);
	for (const auto& InitialShapeClasses : GetInitialShapesClasses())
	{
		UInitialShape* DefaultInitialShape = Cast<UInitialShape>(InitialShapeClasses->GetDefaultObject());
		if (DefaultInitialShape && DefaultInitialShape->CanConstructFrom(this->GetOwner()))
		{
			InitialShape = NewObject<UInitialShape>(GetOwner(), DefaultInitialShape->GetClass(), NAME_None, RF_Transient | RF_TextExportTransient);
		}
	}

	if (!InitialShape)
	{
		InitialShape = NewObject<UInitialShape>(GetOwner(), GetInitialShapesClasses()[0], NAME_None, RF_Transient | RF_TextExportTransient);
	}

	InitialShape->Initialize(this);
}

void UVitruvioComponent::Initialize()
{
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

	// Check if we can load the attributes and then generate (eg during play)
	if (GenerateAutomatically && HasValidInputData())
	{
		EvaluateRuleAttributes(true);
	}
}

void UVitruvioComponent::PostLoad()
{
	Super::PostLoad();

	// Only initialize if the Component is instance (eg via details panel to any Actor)
	if (CreationMethod == EComponentCreationMethod::Instance)
	{
		Initialize();
	}
}

void UVitruvioComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	// Only initialize if the Component is instance (eg via details panel to any Actor)
	if (CreationMethod == EComponentCreationMethod::Instance)
	{
		Initialize();
	}
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

		USceneComponent* InitialShapeComponent = InitialShape->GetComponent();
		UGeneratedModelStaticMeshComponent* VitruvioModelComponent = nullptr;

		TArray<USceneComponent*> InitialShapeChildComponents;
		InitialShapeComponent->GetChildrenComponents(false, InitialShapeChildComponents);
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
					InstanceComponent->Rename(nullptr, GetTransientPackage()); // Remove from Owner
					InstanceComponent->DestroyComponent(false);
				}

				break;
			}
		}

		if (!VitruvioModelComponent)
		{
			VitruvioModelComponent = NewObject<UGeneratedModelStaticMeshComponent>(InitialShapeComponent, FName(TEXT("GeneratedModel")),
																				   RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
			InitialShapeComponent->GetOwner()->AddInstanceComponent(VitruvioModelComponent);
			VitruvioModelComponent->AttachToComponent(InitialShapeComponent, FAttachmentTransformRules::KeepRelativeTransform);
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

		if (bUseHierarchicalInstances)
		{
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
				InitialShapeComponent->GetOwner()->AddInstanceComponent(InstancedComponent);
				InstancedComponent->OnComponentCreated();
				InstancedComponent->RegisterComponent();
			}
		}
		else
		{
			TMap<FString, int32> NameMap;
			for (const FInstance& Instance : ConvertedResult.Instances)
			{
				const TArray<FTransform>& Transforms = Instance.Transforms;
				for (const FTransform& Transform : Transforms)
				{
					FString UniqueName = UniqueComponentName(Instance.Name, NameMap);
					UE_LOG(LogTemp, Warning, TEXT("%s"), *UniqueName);
					auto InstanceComponent = NewObject<UGeneratedModelInstanceComponent>(
						VitruvioModelComponent, FName(UniqueName), RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
					InstanceComponent->SetRelativeTransform(Transform);
					InstanceComponent->SetStaticMesh(Instance.InstanceMesh->GetStaticMesh());
					InstanceComponent->SetCollisionData(Instance.InstanceMesh->GetCollisionData());

					// Apply override materials
					for (int32 MaterialIndex = 0; MaterialIndex < Instance.OverrideMaterials.Num(); ++MaterialIndex)
					{
						InstanceComponent->SetMaterial(MaterialIndex, Instance.OverrideMaterials[MaterialIndex]);
					}

					// Instanced component collision
					CreateCollision(Instance.InstanceMesh->GetStaticMesh(), InstanceComponent, GenerateCollision);

					// Attach and register instance component
					InstanceComponent->AttachToComponent(VitruvioModelComponent, FAttachmentTransformRules::KeepRelativeTransform);
					InitialShapeComponent->GetOwner()->AddInstanceComponent(InstanceComponent);
					InstanceComponent->OnComponentCreated();
					InstanceComponent->RegisterComponent();
				}
			}
		}

		OnHierarchyChanged.Broadcast(this);

		HasGeneratedMesh = true;

		InitialShape->SetHidden(HideAfterGeneration);

		if (Result.CallbackProxy)
		{
			Result.CallbackProxy->OnGenerateCompleted.Broadcast();
			Result.CallbackProxy->OnGenerateCompletedCpp.Broadcast();
			Result.CallbackProxy->SetReadyToDestroy();
		}
		OnGenerateCompleted.Broadcast(this);
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
			AttributesEvaluation.CallbackProxy->OnAttributesEvaluated.Broadcast();
			AttributesEvaluation.CallbackProxy->OnAttributesEvaluatedCpp.Broadcast();
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
	if (!InitialShape)
	{
		return;
	}

	USceneComponent* InitialShapeComponent = InitialShape->GetComponent();

	TArray<USceneComponent*> Children;
	InitialShapeComponent->GetChildrenComponents(true, Children);
	for (USceneComponent* Child : Children)
	{
		Child->DestroyComponent(false);
	}

	HasGeneratedMesh = false;
	InitialShape->SetHidden(false);
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
		auto VitruvioMesh = GenerateResult.Meshes[Instance.Key.PrototypeIndex];
		FString MeshName = Instance.Key.Name;
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
			InitialShape->SetHidden(HideAfterGeneration && HasGeneratedMesh);
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, GenerateAutomatically))
		{
			bComponentPropertyChanged = true;
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVitruvioComponent, bUseHierarchicalInstances))
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

	// If a property has changed which is used for creating the initial shape we have to recreate it
	if (bRecreateInitialShape)
	{
		UInitialShape* DuplicatedInitialShape = DuplicateObject(InitialShape, GetOwner());
		InitialShape->Rename(nullptr, GetTransientPackage()); // Remove from Owner
		InitialShape = DuplicatedInitialShape;
		InitialShape->Initialize(this);

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
		// Force regeneration on attribute undo
		EvaluateRuleAttributes(bIsAttributeUndo);
	}
}

void UVitruvioComponent::SetInitialShapeType(const TSubclassOf<UInitialShape>& Type)
{
	UInitialShape* NewInitialShape = NewObject<UInitialShape>(GetOwner(), Type, NAME_None, RF_Transient | RF_TextExportTransient);

	if (InitialShape)
	{
		if (InitialShape->GetClass() == Type)
		{
			return;
		}

		const FInitialShapePolygon InitialShapePolygon = InitialShape->GetPolygon();
		InitialShape->Uninitialize();

		NewInitialShape->Initialize(this, InitialShapePolygon);
	}
	else
	{
		NewInitialShape->Initialize(this);
	}

	InitialShape->Rename(nullptr, GetTransientPackage()); // Remove from Owner
	InitialShape = NewInitialShape;

	RemoveGeneratedMeshes();
}

#endif // WITH_EDITOR

void UVitruvioComponent::EvaluateRuleAttributes(bool ForceRegenerate, UGenerateCompletedCallbackProxy* CallbackProxy)
{
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
		FScopeLock(&Result.Token->Lock);

		if (Result.Token->IsInvalid())
		{
			return;
		}

		EvalAttributesInvalidationToken.Reset();
		AttributesEvaluationQueue.Enqueue({Result.Value, ForceRegenerate, CallbackProxy});
	});
}

TArray<TSubclassOf<UInitialShape>> UVitruvioComponent::GetInitialShapesClasses()
{
	return {UStaticMeshInitialShape::StaticClass(), USplineInitialShape::StaticClass()};
}
