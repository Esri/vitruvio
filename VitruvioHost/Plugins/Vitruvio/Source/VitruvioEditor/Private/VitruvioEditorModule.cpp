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

#include "VitruvioEditorModule.h"

#include "ChooseRulePackageDialog.h"
#include "RulePackageAssetTypeActions.h"
#include "VitruvioActor.h"
#include "VitruvioComponentDetails.h"
#include "VitruvioCooker.h"

#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "AssetToolsModule.h"
#include "Core.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#include "VitruvioBlueprintLibrary.h"
#include "VitruvioStyle.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "VitruvioEditorModule"

namespace
{

bool HasAnyViableVitruvioActor(TArray<AActor*> Actors)
{
	return Algo::AllOf(Actors, [](AActor* In) { return UVitruvioBlueprintLibrary::CanConvertToVitruvioActor(In); });
}

bool HasAnyVitruvioActor(TArray<AActor*> Actors)
{
	return Algo::AnyOf(Actors, [](AActor* In) {
		UVitruvioComponent* VitruvioComponent = In->FindComponentByClass<UVitruvioComponent>();
		return VitruvioComponent != nullptr;
	});
}

void AssignRulePackage(TArray<AActor*> Actors)
{
	if (Actors.Num() == 0)
	{
		return;
	}

	TOptional<URulePackage*> SelectedRpk = FChooseRulePackageDialog::OpenDialog();

	if (SelectedRpk.IsSet())
	{
		TArray<AVitruvioActor*> ConvertedActors;
		UGenerateCompletedCallbackProxy::ConvertToVitruvioActor(Actors[0], Actors, ConvertedActors, SelectedRpk.GetValue());
	}
}

void SelectAllViableVitruvioActors(TArray<AActor*> Actors)
{
	GEditor->SelectNone(false, true, false);
	for (AActor* SelectedActor : Actors)
	{
		TArray<AActor*> NewSelection = UVitruvioBlueprintLibrary::GetViableVitruvioActorsInHierarchy(SelectedActor);
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

			if (HasAnyVitruvioActor(SelectedActors))
			{
				const FUIAction CookVitruvioActorsAction(FExecuteAction::CreateStatic(CookVitruvioActors, SelectedActors));

				MenuBuilder.AddMenuEntry(FText::FromString("Convert To Static Mesh Actors"),
										 FText::FromString("Converts all selected procedural Vitruvio Actors to Static Mesh Actors."), FSlateIcon(),
										 CookVitruvioActorsAction);
			}

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

void VitruvioEditorModule::PostUndoRedo()
{
	// We want to call PostUndoRedo on the VitruvioComponent after the undo action has completed (note that the overriden PreEditUndo/PostEditUndo on
	// the Component itself is called during the undo operation and always before its owners Actor undo/redo has completed). We also need to check if
	// the VitruvioComponent was involved in the undo/redo action.
	const FTransaction* LastTransaction = GEditor->Trans->GetTransaction(GEditor->Trans->GetQueueLength() - 1);
	TArray<UObject*> TransactionObjects;
	if (LastTransaction)
	{
		LastTransaction->GetTransactionObjects(TransactionObjects);
	}

	const TSet TransactionObjectSet(TransactionObjects);

	for (FActorIterator It(GEditor->GetEditorWorldContext().World()); It; ++It)
	{
		const AActor* Actor = *It;
		UVitruvioComponent* VitruvioComponent = Cast<UVitruvioComponent>(Actor->GetComponentByClass(UVitruvioComponent::StaticClass()));

		if (VitruvioComponent && (TransactionObjectSet.Contains(VitruvioComponent) || TransactionObjectSet.Contains(VitruvioComponent->GetOwner())))
		{
			VitruvioComponent->PostUndoRedo();
		}
	}
}

void VitruvioEditorModule::StartupModule()
{
	FVitruvioStyle::Initialize();

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

	GenerateCompletedDelegateHandle = VitruvioModule::Get().OnAllGenerateCompleted.AddRaw(this, &VitruvioEditorModule::OnGenerateCompleted);

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &VitruvioEditorModule::OnPostEngineInit);

	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	MapChangedHandle = LevelEditor.OnMapChanged().AddRaw(this, &VitruvioEditorModule::OnMapChanged);

	PostUndoRedoDelegate = FEditorDelegates::PostUndoRedo.AddRaw(this, &VitruvioEditorModule::PostUndoRedo);
}

void VitruvioEditorModule::ShutdownModule()
{
	FVitruvioStyle::Shutdown();

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout(UVitruvioComponent::StaticClass()->GetFName());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetAllLevelViewportContextMenuExtenders().RemoveAll(
		[&](const FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors& Delegate) {
			return Delegate.GetHandle() == LevelViewportContextMenuVitruvioExtenderDelegateHandle;
		});

	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	VitruvioModule::Get().OnAllGenerateCompleted.Remove(GenerateCompletedDelegateHandle);
	if (GEditor)
	{
		GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.Remove(OnAssetReloadHandle);
	}

	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnMapChanged().Remove(MapChangedHandle);

	FEditorDelegates::PostUndoRedo.Remove(PostUndoRedoDelegate);
}

void VitruvioEditorModule::OnPostEngineInit()
{
	// clang-format off
	OnAssetReloadHandle = GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.AddLambda([](UObject* Object)
	{
		URulePackage* RulePackage = Cast<URulePackage>(Object);
		if (!RulePackage)
		{
			return;
		}

		VitruvioModule::Get().EvictFromResolveMapCache(RulePackage);

		for (FActorIterator It(GEditor->GetEditorWorldContext().World()); It; ++It)
		{
			AActor* Actor = *It;
			UVitruvioComponent* VitruvioComponent = Cast<UVitruvioComponent>(Actor->GetComponentByClass(UVitruvioComponent::StaticClass()));
			if (VitruvioComponent && VitruvioComponent->GetRpk() == RulePackage)
			{
				VitruvioComponent->RemoveGeneratedMeshes();
				VitruvioComponent->EvaluateRuleAttributes(true);
			}
		}
	});
	// clang-format on
}

void VitruvioEditorModule::OnMapChanged(UWorld* World, EMapChangeType ChangeType)
{
	if (ChangeType == EMapChangeType::TearDownWorld)
	{
		VitruvioModule::Get().GetMeshCache().Empty();

		// Close all open editor of transient meshes generated by Vitruvio to prevent GC issues while loading a new map
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			for (UObject* EditedAsset : AssetEditorSubsystem->GetAllEditedAssets())
			{
				UStaticMesh* StaticMesh = Cast<UStaticMesh>(EditedAsset);
				if (StaticMesh && StaticMesh->GetPackage() == GetTransientPackage())
				{
					AssetEditorSubsystem->CloseAllEditorsForAsset(EditedAsset);
				}
			}
		}
	}
}

void VitruvioEditorModule::OnGenerateCompleted(int NumWarnings, int NumErrors)
{
	FString NotificationText(TEXT("Generate Completed"));
	const FSlateBrush* Image = nullptr;
	if (NumErrors > 0)
	{
		NotificationText += FString::Printf(TEXT(" with %d Errors"), NumErrors);
		Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Error"));
	}
	else if (NumWarnings > 0)
	{
		NotificationText += FString::Printf(TEXT(" with %d Warnings"), NumWarnings);
		Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Warning"));
	}

	FNotificationInfo Info(FText::FromString(NotificationText));

	Info.bFireAndForget = true;
	Info.ExpireDuration = 5.0f;
	Info.Image = Image;

	if (NumWarnings > 0 || NumErrors > 0)
	{
		Info.Hyperlink = FSimpleDelegate::CreateLambda([]() { FGlobalTabmanager::Get()->TryInvokeTab(FName("OutputLog")); });
		Info.HyperlinkText = FText::FromString("Show Output Log");
	}

	if (NotificationItem.IsValid())
	{
		auto NotificationItemPinned = NotificationItem.Pin();
		NotificationItemPinned->SetFadeOutDuration(0);
		NotificationItemPinned->Fadeout();
		NotificationItem.Reset();
	}
	NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(VitruvioEditorModule, VitruvioEditor)
