/* Copyright 2024 Esri
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

#include "CityEngineEditorModule.h"

#include "ConvertToCityEngineActorDialog.h"
#include "RulePackageAssetTypeActions.h"
#include "CityEngineActor.h"
#include "CityEngineComponentDetails.h"
#include "CityEngineActorCooker.h"

#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "AssetToolsModule.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/Transactor.h"
#include "EngineUtils.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GenerateCompletedCallbackProxy.h"
#include "IAssetTools.h"
#include "ReplacementDialog.h"
#include "CityEngineBatchActorDetails.h"
#include "CityEngineBatchGridVisualizerActor.h"
#include "CityEngineBatchSubsystem.h"
#include "Modules/ModuleManager.h"
#include "CityEngineBlueprintLibrary.h"
#include "CityEngineStyle.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "FCityEngineEditorModule"

namespace
{

bool HasAnyViableVitruvioActor(const TArray<AActor*>& Actors)
{
	return Algo::AllOf(Actors, [](AActor* In) { return UCityEngineBlueprintLibrary::CanConvertToCityEngineActor(In); });
}

bool HasAnyVitruvioActor(const TArray<AActor*>& Actors)
{
	return Algo::AnyOf(Actors, [](AActor* In) {
		const UCityEngineComponent* VitruvioComponent = In->FindComponentByClass<UCityEngineComponent>();
		const ACityEngineBatchActor* VitruvioBatchActor = Cast<ACityEngineBatchActor>(In);
		return VitruvioComponent || VitruvioBatchActor;
	});
}

void ConvertToVitruvioActor(TArray<AActor*> Actors)
{
	if (Actors.Num() == 0)
	{
		return;
	}

	TOptional<UConvertOptions*> Options = FConvertToCityEngineActorDialog::OpenDialog();

	if (Options.IsSet())
	{
		TArray<ACityEngineActor*> ConvertedActors;
		UGenerateCompletedCallbackProxy::ConvertToCityEngine(Actors[0], Actors, ConvertedActors, Options.GetValue()->RulePackage, true, Options.GetValue()->bBatchGenerate);
	}
}

void SelectAllInitialShapes(TArray<AActor*> Actors)
{
	GEditor->SelectNone(false, true, false);
	for (AActor* SelectedActor : Actors)
	{
		TArray<AActor*> NewSelection = UCityEngineBlueprintLibrary::GetAttachedInitialShapes(SelectedActor);
		for (AActor* ActorToSelect : NewSelection)
		{
			GEditor->SelectActor(ActorToSelect, true, false);
		}
	}
	GEditor->NoteSelectionChange();
}

void SelectAllVitruvioActors(TArray<AActor*> Actors)
{
	GEditor->SelectNone(false, true, false);
	for (AActor* SelectedActor : Actors)
	{
		TArray<AActor*> NewSelection = UCityEngineBlueprintLibrary::GetAttachedCityEngineActors(SelectedActor);
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
			MenuBuilder.BeginSection("CityEngine", FText::FromString("CityEngine"));

			if (HasAnyViableVitruvioActor(SelectedActors))
			{
				const FUIAction AddVitruvioComponentAction(FExecuteAction::CreateStatic(ConvertToVitruvioActor, SelectedActors));

				MenuBuilder.AddMenuEntry(
					FText::FromString("Convert to CityEngine Actor"),
					FText::FromString("Converts all viable selected Initial Shapes to CityEngine Actors and assigns the chosen Rule Package."),
					FSlateIcon(), AddVitruvioComponentAction);
			}

			if (HasAnyVitruvioActor(SelectedActors))
			{
				const FUIAction CookVitruvioActorsAction(FExecuteAction::CreateStatic(CookCityEngineActors, SelectedActors));

				MenuBuilder.AddMenuEntry(FText::FromString("Convert To Static Mesh Actors"),
										 FText::FromString("Converts all selected procedural CityEngine Actors to Static Mesh Actors."), FSlateIcon(),
										 CookVitruvioActorsAction);
			}

			const FUIAction SelectAllViableVitruvioActorsAction(FExecuteAction::CreateStatic(SelectAllInitialShapes, SelectedActors));
			MenuBuilder.AddMenuEntry(FText::FromString("Select Initial Shapes"),
									 FText::FromString("Select all attached Actors which are viable initial shapes."), FSlateIcon(),
									 SelectAllViableVitruvioActorsAction);

			const FUIAction SelectAllVitruvioActorsAction(FExecuteAction::CreateStatic(SelectAllVitruvioActors, SelectedActors));
			MenuBuilder.AddMenuEntry(FText::FromString("Select CityEngine Actors"),
						 FText::FromString("Selects all attached CityEngine Actors."), FSlateIcon(),
						 SelectAllVitruvioActorsAction);

			MenuBuilder.EndSection();
		}));

	return Extender.ToSharedRef();
}

FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors LevelViewportContextMenuVitruvioExtender;

} // namespace

void FCityEngineEditorModule::PostUndoRedo()
{
	// We want to call PostUndoRedo on the CityEngineComponent after the undo action has completed (note that the overriden PreEditUndo/PostEditUndo on
	// the Component itself is called during the undo operation and always before its owners Actor undo/redo has completed). We also need to check if
	// the CityEngineComponent was involved in the undo/redo action.
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
		UCityEngineComponent* VitruvioComponent = Cast<UCityEngineComponent>(Actor->GetComponentByClass(UCityEngineComponent::StaticClass()));

		if (VitruvioComponent && (TransactionObjectSet.Contains(VitruvioComponent) || TransactionObjectSet.Contains(VitruvioComponent->GetOwner())))
		{
			VitruvioComponent->PostUndoRedo();
		}
	}
}

void FCityEngineEditorModule::StartupModule()
{
	FCityEngineStyle::Initialize();

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FRulePackageAssetTypeActions()));

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(UCityEngineComponent::StaticClass()->GetFName(),
											 FOnGetDetailCustomizationInstance::CreateStatic(&FCityEngineComponentDetails::MakeInstance));

	PropertyModule.RegisterCustomClassLayout(ACityEngineBatchActor::StaticClass()->GetFName(),
										 FOnGetDetailCustomizationInstance::CreateStatic(&FCityEngineBatchActorDetails::MakeInstance));

	LevelViewportContextMenuVitruvioExtender =
		FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateStatic(&ExtendLevelViewportContextMenuForVitruvioComponents);
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	auto& MenuExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
	MenuExtenders.Add(LevelViewportContextMenuVitruvioExtender);
	LevelViewportContextMenuVitruvioExtenderDelegateHandle = MenuExtenders.Last().GetHandle();

	GenerateCompletedDelegateHandle = FCityEngineModule::Get().OnAllGenerateCompleted.AddRaw(this, &FCityEngineEditorModule::OnGenerateCompleted);

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FCityEngineEditorModule::OnPostEngineInit);

	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	MapChangedHandle = LevelEditor.OnMapChanged().AddRaw(this, &FCityEngineEditorModule::OnMapChanged);

	PostUndoRedoDelegate = FEditorDelegates::PostUndoRedo.AddRaw(this, &FCityEngineEditorModule::PostUndoRedo);
}

void FCityEngineEditorModule::ShutdownModule()
{
	FCityEngineStyle::Shutdown();

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout(UCityEngineComponent::StaticClass()->GetFName());
	PropertyModule.UnregisterCustomClassLayout(ACityEngineBatchActor::StaticClass()->GetFName());
	
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetAllLevelViewportContextMenuExtenders().RemoveAll(
		[&](const FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors& Delegate) {
			return Delegate.GetHandle() == LevelViewportContextMenuVitruvioExtenderDelegateHandle;
		});

	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	FCityEngineModule::Get().OnAllGenerateCompleted.Remove(GenerateCompletedDelegateHandle);
	if (GEditor)
	{
		GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.Remove(OnAssetReloadHandle);
	}

	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnMapChanged().Remove(MapChangedHandle);

	FEditorDelegates::PostUndoRedo.Remove(PostUndoRedoDelegate);
}

void FCityEngineEditorModule::BlockUntilGenerated() const
{
	// Wait until all async generate calls to PRT are finished. We want to block the UI and show a modal progress bar.
	int32 TotalGenerateCalls = FCityEngineModule::Get().GetNumGenerateCalls();
	FScopedSlowTask PRTGenerateCallsTasks(TotalGenerateCalls, FText::FromString("Generating models..."));
	PRTGenerateCallsTasks.MakeDialog();
	while (FCityEngineModule::Get().IsGenerating() || FCityEngineModule::Get().IsLoadingRpks())
	{
		FPlatformProcess::Sleep(0); // SwitchToThread
		int32 CurrentNumGenerateCalls = FCityEngineModule::Get().GetNumGenerateCalls();
		PRTGenerateCallsTasks.EnterProgressFrame(TotalGenerateCalls - CurrentNumGenerateCalls);
		TotalGenerateCalls = CurrentNumGenerateCalls;
	}
}

void FCityEngineEditorModule::OnPostEngineInit()
{
	// clang-format off
	OnAssetReloadHandle = GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.AddLambda([](UObject* Object)
	{
		URulePackage* RulePackage = Cast<URulePackage>(Object);
		if (!RulePackage)
		{
			return;
		}

		FCityEngineModule::Get().EvictFromResolveMapCache(RulePackage);

		UCityEngineBatchSubsystem* BatchSubsystem = GEditor->GetEditorWorldContext().World()->GetSubsystem<UCityEngineBatchSubsystem>();
		for (FActorIterator It(GEditor->GetEditorWorldContext().World()); It; ++It)
		{
			AActor* Actor = *It;
			UCityEngineComponent* VitruvioComponent = Cast<UCityEngineComponent>(Actor->GetComponentByClass(UCityEngineComponent::StaticClass()));
			if (VitruvioComponent && VitruvioComponent->GetRpk() == RulePackage)
			{
				if (!VitruvioComponent->IsBatchGenerated())
				{
					VitruvioComponent->RemoveGeneratedMeshes();
					VitruvioComponent->EvaluateRuleAttributes(true);
				}
				else
				{
					BatchSubsystem->Generate(VitruvioComponent);
				}
			}
		}
	});
	// clang-format on
}

void FCityEngineEditorModule::OnMapChanged(UWorld* World, EMapChangeType ChangeType)
{
	if (ChangeType == EMapChangeType::TearDownWorld)
	{
		FCityEngineModule::Get().GetMeshCache().Empty();

		// Close all open editor of transient meshes generated by CityEngine to prevent GC issues while loading a new map
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

		// Close all open replacement dialogs
		TArray<TSharedRef<SWindow>> Windows;
		FSlateApplication::Get().GetAllVisibleWindowsOrdered(Windows);
		for (const auto& Window :  Windows)
		{
			if (Window->GetTag() == "ReplacementDialog")
			{
				Window->RequestDestroyWindow();
			}
		}
	}
	else if (ChangeType == EMapChangeType::LoadMap || ChangeType == EMapChangeType::NewMap)
	{
		if (World)
		{
			UCityEngineBatchSubsystem* VitruvioBatchSubsystem = World->GetSubsystem<UCityEngineBatchSubsystem>();
			VitruvioBatchSubsystem->OnComponentRegistered.AddLambda([World]()
			{
				const TActorIterator<ACityEngineBatchGridVisualizerActor> BatchGridVisualizerActorIter(World);
				if (!BatchGridVisualizerActorIter)
				{
					FActorSpawnParameters ActorSpawnParameters;
					ActorSpawnParameters.Name = FName(TEXT("VitruvioBatchGridVisualizerActor"));
					World->SpawnActor<ACityEngineBatchGridVisualizerActor>(ActorSpawnParameters);
				}
			});
		}
	}
}

void FCityEngineEditorModule::OnGenerateCompleted(int NumWarnings, int NumErrors)
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

IMPLEMENT_MODULE(FCityEngineEditorModule, CityEnginePluginEditor)
