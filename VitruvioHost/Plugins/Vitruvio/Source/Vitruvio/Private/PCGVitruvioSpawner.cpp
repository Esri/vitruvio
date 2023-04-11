// Copyright Epic Games, Inc. All Rights Reserved.

#include "Vitruvio/Public/PCGVitruvioSpawner.h"

#include "Data/PCGPointData.h"
#include "Data/PCGSpatialData.h"
#include "Helpers/PCGActorHelpers.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

UPCGVitruvioSpawnerSettings::UPCGVitruvioSpawnerSettings(const FObjectInitializer& ObjectInitializer)
{
	bUseSeed = true;
}

FPCGElementPtr UPCGVitruvioSpawnerSettings::CreateElement() const
{
	return MakeShared<FPCGVitruvioSpawnerElement>();
}

bool FPCGVitruvioSpawnerElement::PrepareDataInternal(FPCGContext* InContext) const
{
	return true;
}

bool FPCGVitruvioSpawnerElement::ExecuteInternal(FPCGContext* InContext) const
{
	return true;
}

FPCGContext* FPCGVitruvioSpawnerElement::Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent,
													const UPCGNode* Node)
{
	FPCGVitruvioSpawnerContext* Context = new FPCGVitruvioSpawnerContext();
	Context->InputData = InputData;
	Context->SourceComponent = SourceComponent;
	Context->Node = Node;

	return Context;
}

bool FPCGVitruvioSpawnerElement::CanExecuteOnlyOnMainThread(FPCGContext* Context) const
{
	return Context->CurrentPhase == EPCGExecutionPhase::Execute;
}

void UPCGVitruvioSpawnerSettings::PostLoad()
{
	Super::PostLoad();
}
