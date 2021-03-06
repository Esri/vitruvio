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

void AssignRulePackage(TArray<AActor*> Actors)
{
	TOptional<URulePackage*> SelectedRpk = FChooseRulePackageDialog::OpenDialog();

	if (SelectedRpk.IsSet())
	{
		URulePackage* Rpk = SelectedRpk.GetValue();

		UVitruvioComponent* Component = nullptr;
		for (AActor* Actor : Actors)
		{
			AActor* OldAttachParent = Actor->GetAttachParentActor();
			if (Actor->IsA<AStaticMeshActor>())
			{
				AVitruvioActor* VitruvioActor = Actor->GetWorld()->SpawnActor<AVitruvioActor>(Actor->GetActorLocation(), Actor->GetActorRotation());

				UStaticMeshComponent* OldStaticMeshComponent = Actor->FindComponentByClass<UStaticMeshComponent>();

				UStaticMeshComponent* NewStaticMeshComponent = NewObject<UStaticMeshComponent>(VitruvioActor, TEXT("InitialShapeStaticMesh"));
				NewStaticMeshComponent->Mobility = EComponentMobility::Movable;
				NewStaticMeshComponent->SetStaticMesh(OldStaticMeshComponent->GetStaticMesh());
				NewStaticMeshComponent->SetWorldTransform(VitruvioActor->GetTransform());
				VitruvioActor->AddInstanceComponent(NewStaticMeshComponent);
				NewStaticMeshComponent->AttachToComponent(VitruvioActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
				NewStaticMeshComponent->OnComponentCreated();
				NewStaticMeshComponent->RegisterComponent();

				UVitruvioComponent* VitruvioComponent = VitruvioActor->VitruvioComponent;
				VitruvioComponent->SetRpk(Rpk);

				VitruvioActor->Initialize();

				if (OldAttachParent)
				{
					VitruvioActor->AttachToActor(OldAttachParent, FAttachmentTransformRules::KeepWorldTransform);
				}

				Actor->Destroy();
			}
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
				const FUIAction AddVitruvioComponentAction(FExecuteAction::CreateStatic(AssignRulePackage, SelectedActors));

				MenuBuilder.AddMenuEntry(
					FText::FromString("Convert to Vitruvio Actor"),
					FText::FromString("Converts all viable selected Initial Shapes to Vitruvio Actors and assigns the chosen Rule Package."),
					FSlateIcon(), AddVitruvioComponentAction);
			}

			MenuBuilder.EndSection();
		}));

	Extender->AddMenuExtension(
		"SelectMatinee", EExtensionHook::After, CommandList, FMenuExtensionDelegate::CreateLambda([SelectedActors](FMenuBuilder& MenuBuilder) {
			MenuBuilder.BeginSection("SelectPossibleVitruvio", FText::FromString("Vitruvio"));

			const FUIAction SelectAllViableVitruvioActorsAction(FExecuteAction::CreateStatic(SelectAllViableVitruvioActors, SelectedActors));
			MenuBuilder.AddMenuEntry(FText::FromString("Select All Viable Initial Shapes In Hierarchy"),
									 FText::FromString("Selects all Actors which are viable initial shapes in hierarchy."), FSlateIcon(),
									 SelectAllViableVitruvioActorsAction);

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
