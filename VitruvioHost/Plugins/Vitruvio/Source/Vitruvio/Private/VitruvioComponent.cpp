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

FVector GetCentroid(const TArray<FVector>& Vertices)
{
	FVector Centroid = FVector::ZeroVector;
	for (const FVector& Vertex : Vertices)
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
		return false;
	}

	URuleAttribute const* Attribute = *FoundAttribute;
	A const* TAttribute = Cast<A>(Attribute);
	if (!TAttribute)
	{
		return false;
	}

	OutValue = TAttribute->Value;

	return true;
}

template <typename A, typename T>
bool SetAttribute(UVitruvioComponent* VitruvioComponent, TMap<FString, URuleAttribute*>& Attributes, const FString& Name, const T& Value)
{
	URuleAttribute** FoundAttribute = Attributes.Find(Name);
	if (!FoundAttribute)
	{
		return false;
	}

	URuleAttribute* Attribute = *FoundAttribute;
	A* TAttribute = Cast<A>(Attribute);
	if (!TAttribute)
	{
		return false;
	}

	TAttribute->Value = Value;
	if (VitruvioComponent->GenerateAutomatically && VitruvioComponent->IsReadyToGenerate())
	{
		VitruvioComponent->Generate();
	}

	return true;
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
		const FVector Centroid = GetCentroid(InitialShape->GetVertices());
		RandomSeed = GetTypeHash(GetOwner()->GetActorTransform().TransformPosition(Centroid));
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

void UVitruvioComponent::SetRpk(URulePackage* RulePackage)
{
	if (this->Rpk == RulePackage)
	{
		return;
	}

	this->Rpk = RulePackage;

	Attributes.Empty();
	bAttributesReady = false;
	bNotifyAttributeChange = true;
}

bool UVitruvioComponent::SetStringAttribute(const FString& Name, const FString& Value)
{
	return SetAttribute<UStringAttribute, FString>(this, this->Attributes, Name, Value);
}

bool UVitruvioComponent::GetStringAttribute(const FString& Name, FString& OutValue) const
{
	return GetAttribute<UStringAttribute, FString>(this->Attributes, Name, OutValue);
}

bool UVitruvioComponent::SetBoolAttribute(const FString& Name, bool Value)
{
	return SetAttribute<UBoolAttribute, bool>(this, this->Attributes, Name, Value);
}

bool UVitruvioComponent::GetBoolAttribute(const FString& Name, bool& OutValue) const
{
	return GetAttribute<UBoolAttribute, bool>(this->Attributes, Name, OutValue);
}

bool UVitruvioComponent::SetFloatAttribute(const FString& Name, float Value)
{
	return SetAttribute<UFloatAttribute, float>(this, this->Attributes, Name, Value);
}

bool UVitruvioComponent::GetFloatAttribute(const FString& Name, float& OutValue) const
{
	return GetAttribute<UFloatAttribute, float>(this->Attributes, Name, OutValue);
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
		FGenerateResultDescription Result;
		GenerateQueue.Dequeue(Result);

		FConvertedGenerateResult ConvertedResult =
			BuildResult(Result, VitruvioModule::Get().GetMaterialCache(), VitruvioModule::Get().GetTextureCache());

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
					InstanceComponent->DestroyComponent(true);
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

		OnHierarchyChanged.Broadcast(this);

		HasGeneratedMesh = true;

		InitialShape->SetHidden(HideAfterGeneration);
	}
}

void UVitruvioComponent::ProcessAttributesEvaluationQueue()
{
	if (!AttributesEvaluationQueue.IsEmpty())
	{
		FAttributesEvaluation AttributesEvaluation;
		AttributesEvaluationQueue.Dequeue(AttributesEvaluation);

		AttributesEvaluation.AttributeMap->UpdateUnrealAttributeMap(Attributes, this);
		
		bAttributesReady = true;
		bNotifyAttributeChange = true;

		if (GenerateAutomatically || AttributesEvaluation.bForceRegenerate)
		{
			Generate();
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
		Child->DestroyComponent(true);
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

void UVitruvioComponent::Generate()
{
	// If the initial shape and RPK are valid but we have not yet loaded the attributes we load the attributes
	// and regenerate afterwards
	if (HasValidInputData() && !bAttributesReady)
	{
		EvaluateRuleAttributes(true);
		return;
	}

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
		GenerateToken->RequestRegenerate();

		return;
	}

	if (InitialShape)
	{
		FGenerateResult GenerateResult =
			VitruvioModule::Get().GenerateAsync(InitialShape->GetFaces(), Rpk, Vitruvio::CreateAttributeMap(Attributes), RandomSeed);

		GenerateToken = GenerateResult.Token;

		// clang-format off
		GenerateResult.Result.Next([this](const FGenerateResult::ResultType& Result)
		{
			FScopeLock Lock(&Result.Token->Lock);

			if (Result.Token->IsInvalid()) {
				return;
			}

			GenerateToken.Reset();
			if (Result.Token->IsRegenerateRequested())
			{
				Generate();
			}
			else
			{
				GenerateQueue.Enqueue(Result.Value);
			}
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
	}

	// If an object was changed via an undo command, the PropertyChangedEvent.Property is null
	// Therefore, we can only check the ObjectType and check if the Property is null (=likely undo event)
	// This is suboptimal, since it can also happen during undo commands on irrelevant properties of that object. Might be improved later...
	const bool bIsSplinePropertyUndo = Object->IsA(USplineComponent::StaticClass()) && PropertyChangedEvent.Property == nullptr;
	const bool bIsAttributeUndo = Object->IsA(URuleAttribute::StaticClass()) && PropertyChangedEvent.Property == nullptr;

	const bool bRelevantProperty = InitialShape != nullptr && (
		                               bIsSplinePropertyUndo || InitialShape->IsRelevantProperty(Object, PropertyChangedEvent));
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
		//Force regeneration on attribute undo
		EvaluateRuleAttributes(bIsAttributeUndo);
	}
}

void UVitruvioComponent::SetInitialShapeType(const TSubclassOf<UInitialShape>& Type)
{
	UInitialShape* NewInitialShape = NewObject<UInitialShape>(GetOwner(), Type, NAME_None, RF_Transient | RF_TextExportTransient);

	if (InitialShape)
	{
		const TArray<FInitialShapeFace> Faces = InitialShape->GetFaces();
		InitialShape->Uninitialize();

		NewInitialShape->Initialize(this, Faces);
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

void UVitruvioComponent::EvaluateRuleAttributes(bool ForceRegenerate)
{
	check(Rpk);
	check(InitialShape);

	// Since we can not abort an ongoing generate call from PRT, we invalidate the result and evaluate the attributes again after the current request
	// has completed.
	if (EvalAttributesInvalidationToken)
	{
		EvalAttributesInvalidationToken->RequestReEvaluateAttributes();
		return;
	}

	bAttributesReady = false;

	FAttributeMapResult AttributesResult = VitruvioModule::Get().EvaluateRuleAttributesAsync(InitialShape->GetFaces(), Rpk, Vitruvio::CreateAttributeMap(Attributes), RandomSeed);

	EvalAttributesInvalidationToken = AttributesResult.Token;

	AttributesResult.Result.Next([this, ForceRegenerate](const FAttributeMapResult::ResultType& Result) {
		FScopeLock(&Result.Token->Lock);

		if (Result.Token->IsInvalid())
		{
			return;
		}
		
		EvalAttributesInvalidationToken.Reset();
		if (Result.Token->IsReEvaluateRequested())
		{
			EvaluateRuleAttributes(ForceRegenerate);
		}
		else
		{
			AttributesEvaluationQueue.Enqueue({Result.Value, ForceRegenerate});
		}
	});
}

TArray<TSubclassOf<UInitialShape>> UVitruvioComponent::GetInitialShapesClasses()
{
	return {UStaticMeshInitialShape::StaticClass(), USplineInitialShape::StaticClass()};
}
