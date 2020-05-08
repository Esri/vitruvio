// Fill out your copyright notice in the Description page of Project Settings.

#include "PRTActor.h"

#include "ObjectEditorUtils.h"
#include "VitruvioModule.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

APRTActor::APRTActor()
{
	PrimaryActorTick.bCanEverTick = true;
	
	OpaqueParent = LoadObject<UMaterial>(this, TEXT("Material'/Vitruvio/Materials/M_OpaqueParent.M_OpaqueParent'"), nullptr);
	MaskedParent = LoadObject<UMaterial>(this, TEXT("Material'/Vitruvio/Materials/M_MaskedParent.M_MaskedParent'"), nullptr);
	TranslucentParent = LoadObject<UMaterial>(this, TEXT("Material'/Vitruvio/Materials/M_TranslucentParent.M_TranslucentParent'"), nullptr);
}

void APRTActor::BeginPlay()
{
	Super::BeginPlay();
}

void APRTActor::LoadDefaultAttributes(UStaticMesh* InitialShape)
{
	check(InitialShape);
	check(Rpk);
	
	AttributesReady = false;
	
	TFuture<TMap<FString, URuleAttribute*>> AttributesFuture = VitruvioModule::Get().LoadDefaultRuleAttributesAsync(InitialShape, Rpk);
	AttributesFuture.Next([this](const TMap<FString, URuleAttribute*>& Result) {
		
		Attributes = Result;
		AttributesReady = true;

#if WITH_EDITOR
		// Notify possible listeners (eg. Details panel) about changes to the Attributes
		FFunctionGraphTask::CreateAndDispatchWhenReady([this, &Result]()
		{
		    FPropertyChangedEvent PropertyEvent(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(APRTActor, Attributes)));
			FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(this, PropertyEvent);
		}, TStatId(), nullptr, ENamedThreads::GameThread);
#endif
		
		if (GenerateAutomatically)
		{
			Regenerate();
		}
	});
}

void APRTActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Regenerated = false; 
	
	// Note that we also tick in editor for initialization
	if (!Initialized)
	{
		// Load default values for generate attributes if they have not been set
		UStaticMesh* InitialShape = GetStaticMeshComponent()->GetStaticMesh();
		if (Rpk && InitialShape)
		{
			LoadDefaultAttributes(InitialShape);
		}

		Initialized = true;
	}
}

void APRTActor::Regenerate()
{
	if (Rpk && AttributesReady && !Regenerated)
	{
		Regenerated = true;
		
		// Generate
		if (GetStaticMeshComponent())
		{
			UStaticMesh* InitialShape = GetStaticMeshComponent()->GetStaticMesh();

			if (InitialShape)
			{
				VitruvioModule::Get().GenerateAsync(InitialShape, OpaqueParent, MaskedParent, TranslucentParent, Rpk, Attributes)
				.Next([=](const FGenerateResult& Result)
				{
					const FGraphEventRef CreateMeshTask = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &Result]()
					{
						// Remove previously generated actors
						TArray<AActor*> GeneratedMeshes;
						GetAttachedActors(GeneratedMeshes);
						for (const auto& Child : GeneratedMeshes)
						{
							Child->Destroy();
						}

						// Create actors for generated meshes
						QUICK_SCOPE_CYCLE_COUNTER(STAT_PRTActor_CreateActors);
						FActorSpawnParameters Parameters;
						Parameters.Owner = this;
						AStaticMeshActor* StaticMeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(Parameters);
						StaticMeshActor->SetMobility(EComponentMobility::Movable);
						StaticMeshActor->GetStaticMeshComponent()->SetStaticMesh(Result.ShapeMesh);
						StaticMeshActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

						for (const auto Instance : Result.Instances)
						{
							auto InstancedComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(StaticMeshActor);
							InstancedComponent->SetStaticMesh(Instance.Key);
							for (FTransform InstanceTransform : Instance.Value)
							{
								InstancedComponent->AddInstance(InstanceTransform);
							}
							StaticMeshActor->AddInstanceComponent(InstancedComponent);
							InstancedComponent->RegisterComponent();
							InstancedComponent->SetRelativeTransform(StaticMeshActor->GetTransform());
						}
						StaticMeshActor->RegisterAllComponents();
						
					}, TStatId(), nullptr, ENamedThreads::GameThread);
					
					FTaskGraphInterface::Get().WaitUntilTaskCompletes(CreateMeshTask);
				});
			}
		}
	}
}

#if WITH_EDITOR
void APRTActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(APRTActor, Rpk))
	{
		Attributes.Empty();

		UStaticMesh* InitialShape = GetStaticMeshComponent()->GetStaticMesh();
		if (Rpk && InitialShape)
		{
			LoadDefaultAttributes(InitialShape);
		}
	}

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == L"StaticMeshComponent")
	{
		UStaticMesh* InitialShape = GetStaticMeshComponent()->GetStaticMesh();
		if (InitialShape)
		{
			InitialShape->bAllowCPUAccess = true;

			if (Rpk)
			{
				LoadDefaultAttributes(InitialShape);
			}
		}
	}
	
	if (GenerateAutomatically)
	{
		Regenerate();
	}
}

bool APRTActor::ShouldTickIfViewportsOnly() const
{
	return true;
}
#endif
