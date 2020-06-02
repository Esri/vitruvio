// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "PRTActor.h"

#include "VitruvioModule.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ObjectEditorUtils.h"

namespace
{
	int32 CalculateRandomSeed(const FTransform Transform, UStaticMesh* const InitialShape)
	{
		FVector Centroid = FVector::ZeroVector;
		if (InitialShape->RenderData != nullptr && InitialShape->RenderData->LODResources.IsValidIndex(0))
		{
			const FStaticMeshLODResources& LOD = InitialShape->RenderData->LODResources[0];
			for (auto SectionIndex = 0; SectionIndex < LOD.Sections.Num(); ++SectionIndex)
			{
				for (uint32 VertexIndex = 0; VertexIndex < LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++VertexIndex)
				{
					Centroid += LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex);
				}
			}
			Centroid = Centroid / LOD.GetNumVertices();
		}
		return GetTypeHash(Transform.TransformPosition(Centroid));
	}
} // namespace

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

void APRTActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

void APRTActor::Generate()
{
	if (!Rpk || !AttributesReady || !GetStaticMeshComponent())
	{
		return;
	}

	// Since we can not abort an ongoing generate call from PRT, we just regenerate after the current generate call has completed.
	if (bIsGenerating)
	{
		bNeedsRegenerate = true;
		return;
	}

	bIsGenerating = true;

	UStaticMesh* InitialShape = GetStaticMeshComponent()->GetStaticMesh();

	if (InitialShape)
	{
		TFuture<FGenerateResult> GenerateFuture = VitruvioModule::Get().GenerateAsync(InitialShape, OpaqueParent, MaskedParent, TranslucentParent, Rpk, Attributes, RandomSeed);

		// clang-format off
		GenerateFuture.Next([=](const FGenerateResult& Result)
		{
			const FGraphEventRef CreateMeshTask = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &Result]()
			{
				bIsGenerating = false;
				
				// If we need a regenerate (eg there has been a generate request while there 
				if (bNeedsRegenerate)
				{
					bNeedsRegenerate = false;
					Generate();
				}
				else
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

					for (const auto& Instance : Result.Instances)
					{
						auto InstancedComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(StaticMeshActor);
						InstancedComponent->SetStaticMesh(Instance.Key);
						for (const FTransform& InstanceTransform : Instance.Value)
						{
							InstancedComponent->AddInstance(InstanceTransform);
						}
						InstancedComponent->AttachToComponent(StaticMeshActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
						StaticMeshActor->AddInstanceComponent(InstancedComponent);
					}
					StaticMeshActor->RegisterAllComponents();

					bNeedsRegenerate = false;
				}
				
			},
			TStatId(), nullptr, ENamedThreads::GameThread);

			FTaskGraphInterface::Get().WaitUntilTaskCompletes(CreateMeshTask);
		});
		// clang-format on
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

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(APRTActor, RandomSeed))
	{
		bValidRandomSeed = true;
	}

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == TEXT("StaticMeshComponent"))
	{
		UStaticMesh* InitialShape = GetStaticMeshComponent()->GetStaticMesh();
		if (InitialShape)
		{
			InitialShape->bAllowCPUAccess = true;
			if (Rpk)
			{
				LoadDefaultAttributes(InitialShape);
			}

			if (!bValidRandomSeed)
			{
				RandomSeed = CalculateRandomSeed(GetActorTransform(), InitialShape);
				bValidRandomSeed = true;
			}
		}
	}

	if (GenerateAutomatically)
	{
		Generate();
	}
}

bool APRTActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

#endif // WITH_EDITOR

void APRTActor::LoadDefaultAttributes(UStaticMesh* InitialShape)
{
	check(InitialShape);
	check(Rpk);

	AttributesReady = false;

	TFuture<TMap<FString, URuleAttribute*>> AttributesFuture = VitruvioModule::Get().LoadDefaultRuleAttributesAsync(InitialShape, Rpk, RandomSeed);
	AttributesFuture.Next([this](const TMap<FString, URuleAttribute*>& Result) {
		Attributes = Result;
		AttributesReady = true;

		FGenericPlatformMisc::MemoryBarrier();

#if WITH_EDITOR
		// Notify possible listeners (eg. Details panel) about changes to the Attributes
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[this, &Result]() {
				FPropertyChangedEvent PropertyEvent(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(APRTActor, Attributes)));
				FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(this, PropertyEvent);
			},
			TStatId(), nullptr, ENamedThreads::GameThread);
#endif // WITH_EDITOR

		if (GenerateAutomatically)
		{
			Generate();
		}
	});
}
