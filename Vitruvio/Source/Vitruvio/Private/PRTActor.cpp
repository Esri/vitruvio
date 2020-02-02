// Fill out your copyright notice in the Description page of Project Settings.

#include "PRTActor.h"
#include "UnrealGeometryEncoderModule.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

APRTActor::APRTActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APRTActor::BeginPlay()
{
	Super::BeginPlay();
}

void APRTActor::Tick(float DeltaTime)
{
	// Note that we also tick in editor for initialization
	if (!Initialized)
	{
		// Load default values for generate attributes if they have not been set
		if (Rpk)
		{
			UStaticMesh* InitialShape = GetStaticMeshComponent()->GetStaticMesh();
			UnrealGeometryEncoderModule::Get().LoadDefaultRuleAttributes(InitialShape, Rpk, GenerateAttributes);
		}

		if (GenerateAutomatically)
		{
			Regenerate();
		}

		Initialized = true;
	}
}

void APRTActor::Regenerate()
{
	if (Rpk)
	{
		// TODO check for type
		// Remove old prt meshes
		TArray<AActor*> GeneratedMeshes;
		GetAttachedActors(GeneratedMeshes);
		for (const auto& Child : GeneratedMeshes)
		{
			Child->Destroy();
		}

		// Generate
		if (GetStaticMeshComponent())
		{
			UStaticMesh* InitialShape = GetStaticMeshComponent()->GetStaticMesh();

			if (InitialShape)
			{
				FGenerateResult GenerateResult = UnrealGeometryEncoderModule::Get().Generate(InitialShape, Rpk, GenerateAttributes);

				FActorSpawnParameters Parameters;
				Parameters.Owner = this;
				AStaticMeshActor* StaticMeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(Parameters);
				StaticMeshActor->SetMobility(EComponentMobility::Movable);
				StaticMeshActor->GetStaticMeshComponent()->SetStaticMesh(GenerateResult.ShapeMesh);
				StaticMeshActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

				for (const auto Instance : GenerateResult.Instances)
				{
					auto InstancedComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(StaticMeshActor);
					InstancedComponent->SetStaticMesh(Instance.Key);
					for (FTransform InstanceTransform : Instance.Value)
					{
						InstancedComponent->AddInstance(InstanceTransform);
					}
					InstancedComponent->RegisterComponent();
					StaticMeshActor->AddInstanceComponent(InstancedComponent);
				}
			}
		}
	}
}

#ifdef WITH_EDITOR
void APRTActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(APRTActor, Rpk))
	{
		GenerateAttributes.Empty();
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
