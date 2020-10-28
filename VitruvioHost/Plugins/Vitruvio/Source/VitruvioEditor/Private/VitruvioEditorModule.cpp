// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "VitruvioEditorModule.h"

#include "RulePackageAssetTypeActions.h"
#include "VitruvioComponentDetails.h"

#include "AssetToolsModule.h"
#include "Core.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "VitruvioEditorModule"

#include "Algo/AnyOf.h"
#include "ChooseRulePackageDialog.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "VitruvioActor.h"

namespace
{

bool IsViableVitruvioActor(AActor* Actor)
{
	for (const auto& InitialShapeClasses : UVitruvioComponent::GetInitialShapesClasses())
	{
		UInitialShape* DefaultInitialShape = Cast<UInitialShape>(InitialShapeClasses->GetDefaultObject());
		if (DefaultInitialShape && DefaultInitialShape->CanConstructFrom(Actor))
		{
			return true;
		}
	}
	return false;
}

bool HasAnyViableVitruvioActor(TArray<AActor*> Actors)
{
	return Algo::AnyOf(Actors, [](AActor* In) { return IsViableVitruvioActor(In); });
}

TArray<AActor*> GetViableVitruvioActorsInHiararchy(AActor* Root)
{
	TArray<AActor*> ViableActors;
	if (IsViableVitruvioActor(Root))
	{
		ViableActors.Add(Root);
	}

	// If the actor has a VitruvioComponent attached we do not further check its children.
	if (Root->FindComponentByClass<UVitruvioComponent>() == nullptr)
	{
		TArray<AActor*> ChildActors;
		Root->GetAttachedActors(ChildActors);

		for (AActor* Child : ChildActors)
		{
			ViableActors.Append(GetViableVitruvioActorsInHiararchy(Child));
		}
	}

	return ViableActors;
}

void ConvertToVitruvioActors(TArray<AActor*> Actors)
{
	GEditor->SelectNone(true, true, false);

	for (AActor* Actor : Actors)
	{
		UVitruvioComponent* Component = Actor->FindComponentByClass<UVitruvioComponent>();
		if (Component && Actor->IsA<AStaticMeshActor>())
		{
			AActor* VitruvioActor = Actor->GetWorld()->SpawnActor<AActor>(Actor->GetActorLocation(), Actor->GetActorRotation());

			// Root
			USceneComponent* RootComponent =
				NewObject<USceneComponent>(VitruvioActor, USceneComponent::GetDefaultSceneRootVariableName(), RF_Transactional);
			RootComponent->Mobility = EComponentMobility::Movable;
			RootComponent->SetWorldTransform(Actor->GetTransform());
			VitruvioActor->SetRootComponent(RootComponent);
			VitruvioActor->AddInstanceComponent(RootComponent);

			// StaticMesh
			UStaticMeshComponent* OldStaticMeshComponent = Actor->FindComponentByClass<UStaticMeshComponent>();
			UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(VitruvioActor, TEXT("InitialShapeStaticMesh"));
			StaticMeshComponent->SetStaticMesh(OldStaticMeshComponent->GetStaticMesh());
			StaticMeshComponent->Mobility = EComponentMobility::Movable;
			VitruvioActor->AddInstanceComponent(StaticMeshComponent);
			StaticMeshComponent->AttachToComponent(VitruvioActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			StaticMeshComponent->OnComponentCreated();
			StaticMeshComponent->RegisterComponent();

			// Vitruvio
			UVitruvioComponent* VitruvioComponent = DuplicateObject(Component, VitruvioActor);
			VitruvioComponent->OnComponentCreated();
			VitruvioComponent->RegisterComponent();

			TArray<AActor*> AttachedActors;
			Actor->GetAttachedActors(AttachedActors);
			for (AActor* Attached : AttachedActors)
			{
				Attached->Destroy();
			}

			Actor->Destroy();
			GEditor->SelectActor(VitruvioActor, true, true);
			GEditor->SelectComponent(VitruvioComponent, true, true);
		}
	}
	GEditor->NoteSelectionChange();
}

bool CanConvertAnyToVitruvioActor(TArray<AActor*> Actors)
{
	return Algo::AnyOf(Actors, [](AActor* In) {
		UVitruvioComponent* Component = In->FindComponentByClass<UVitruvioComponent>();
		if (Component && In->IsA<AStaticMeshActor>())
		{
			return true;
		}
		return false;
	});
}

bool HasAnyVitruvioComponent(TArray<AActor*> Actors)
{
	return Algo::AnyOf(Actors, [](AActor* In) {
		UVitruvioComponent* Component = In->FindComponentByClass<UVitruvioComponent>();
		if (Component)
		{
			return true;
		}
		return false;
	});
}

void SwitchRpk(TArray<AActor*> Actors)
{
	TOptional<URulePackage*> SelectedRpk = FChooseRulePackageDialog::OpenDialog();

	if (SelectedRpk.IsSet())
	{
		URulePackage* Rpk = SelectedRpk.GetValue();

		for (AActor* Actor : Actors)
		{
			if (UVitruvioComponent* Component = Actor->FindComponentByClass<UVitruvioComponent>())
			{
				Component->SetRpk(Rpk);
				Component->Generate();
			}
		}
	}
}

void AddVitruvioComponents(TArray<AActor*> Actors)
{
	TOptional<URulePackage*> SelectedRpk = FChooseRulePackageDialog::OpenDialog();

	if (SelectedRpk.IsSet())
	{
		URulePackage* Rpk = SelectedRpk.GetValue();

		UVitruvioComponent* Component = nullptr;
		for (AActor* Actor : Actors)
		{
			if (!Actor->FindComponentByClass<UVitruvioComponent>())
			{
				Component = NewObject<UVitruvioComponent>(Actor, TEXT("VitruvioComponent"));
				Actor->AddInstanceComponent(Component);
				Component->OnComponentCreated();
				Component->RegisterComponent();

				Component->SetRpk(Rpk);
				Component->Generate();
			}
		}

		// Select the last component and reselect its Actor. This will not deselect any other actor but "force update"
		// the details panel of the current selected actor details panel.
		if (Component)
		{
			GEditor->SelectActor(Component->GetOwner(), true, true);
			GEditor->SelectComponent(Component, true, true);
		}
	}
}

void SelectAllViableVitruvioActors(TArray<AActor*> Actors)
{
	GEditor->SelectNone(false, true, false);
	for (AActor* SelectedActor : Actors)
	{
		TArray<AActor*> NewSelection = GetViableVitruvioActorsInHiararchy(SelectedActor);
		for (AActor* ActorToSelect : NewSelection)
		{
			GEditor->SelectActor(ActorToSelect, true, false);
		}
	}
	GEditor->NoteSelectionChange();
}

TSharedRef<FExtender> ExtendLevelViewportContextMenuForVitruvioComponents(const TSharedRef<FUICommandList> CommandList,
																		  TArray<AActor*> SelectedActors)
{
	TSharedPtr<FExtender> Extender = MakeShareable(new FExtender);

	Extender->AddMenuExtension(
		"ActorControl", EExtensionHook::After, CommandList, FMenuExtensionDelegate::CreateLambda([SelectedActors](FMenuBuilder& MenuBuilder) {
			MenuBuilder.BeginSection("Vitruvio", FText::FromString("Vitruvio"));

			if (HasAnyViableVitruvioActor(SelectedActors))
			{
				const FUIAction AddVitruvioComponentAction(FExecuteAction::CreateStatic(AddVitruvioComponents, SelectedActors));

				MenuBuilder.AddMenuEntry(FText::FromString("Add Vitruvio Component"),
										 FText::FromString("Adds Vitruvio Components to the selected Actors"), FSlateIcon(),
										 AddVitruvioComponentAction);
			}

			if (HasAnyVitruvioComponent(SelectedActors))
			{
				const FUIAction SwitchRpkAction(FExecuteAction::CreateStatic(SwitchRpk, SelectedActors));

				MenuBuilder.AddMenuEntry(FText::FromString("Change Rule Package"),
										 FText::FromString("Changes the Rule Package of all selected Vitruvio Actors"), FSlateIcon(),
										 SwitchRpkAction);
			}

			if (CanConvertAnyToVitruvioActor(SelectedActors))
			{
				const FUIAction SwitchToNormalActor(FExecuteAction::CreateStatic(ConvertToVitruvioActors, SelectedActors));

				MenuBuilder.AddMenuEntry(FText::FromString("Convert StaticMeshActor to VitruvioActor"),
										 FText::FromString("Converts all selected StaticMeshActors to VitruvioActors"), FSlateIcon(),
										 SwitchToNormalActor);
			}

			MenuBuilder.EndSection();
		}));

	Extender->AddMenuExtension(
		"SelectMatinee", EExtensionHook::After, CommandList, FMenuExtensionDelegate::CreateLambda([SelectedActors](FMenuBuilder& MenuBuilder) {
			MenuBuilder.BeginSection("SelectPossibleVitruvio", FText::FromString("Vitruvio"));

			const FUIAction SelectAllViableVitruvioActorsAction(FExecuteAction::CreateStatic(SelectAllViableVitruvioActors, SelectedActors));
			MenuBuilder.AddMenuEntry(FText::FromString("Select All Viable Vitruvio Actors In Hiararchy"),
									 FText::FromString("Selects all Actors which are viable to attach VitruvioComponents to in hiararchy."),
									 FSlateIcon(), SelectAllViableVitruvioActorsAction);

			MenuBuilder.EndSection();
		}));

	return Extender.ToSharedRef();
}

FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors LevelViewportContextMenuVitruvioExtender;

} // namespace

void VitruvioEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FRulePackageAssetTypeActions()));

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(UVitruvioComponent::StaticClass()->GetFName(),
											 FOnGetDetailCustomizationInstance::CreateStatic(&FVitruvioComponentDetails::MakeInstance));

	LevelViewportContextMenuVitruvioExtender =
		FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateStatic(&ExtendLevelViewportContextMenuForVitruvioComponents);
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	auto& MenuExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
	MenuExtenders.Add(LevelViewportContextMenuVitruvioExtender);
	LevelViewportContextMenuVitruvioExtenderDelegateHandle = MenuExtenders.Last().GetHandle();
}

void VitruvioEditorModule::ShutdownModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout(UVitruvioComponent::StaticClass()->GetFName());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetAllLevelViewportContextMenuExtenders().RemoveAll(
		[&](const FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors& Delegate) {
			return Delegate.GetHandle() == LevelViewportContextMenuVitruvioExtenderDelegateHandle;
		});
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(VitruvioEditorModule, VitruvioEditor)
