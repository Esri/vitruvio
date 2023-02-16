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

#pragma once

#include "Modules/ModuleManager.h"

class UVitruvioComponent;

class VitruvioEditorModule final : public IModuleInterface, public FOutputDevice
{
public:
	void StartupModule() override;
	void ShutdownModule() override;

private:
	void OnPostEngineInit();
	void PostUndoRedo();
	void OnMapChanged(UWorld* World, EMapChangeType ChangeType);
	void OnGenerateCompleted(int NumWarnings, int NumErrors);

public:
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override;

private:
	TWeakPtr<SNotificationItem> NotificationItem;

	FDelegateHandle LevelViewportContextMenuVitruvioExtenderDelegateHandle;
	FDelegateHandle GenerateCompletedDelegateHandle;
	FDelegateHandle OnAssetReloadHandle;
	FDelegateHandle MapChangedHandle;
	FDelegateHandle PostUndoRedoDelegate;
};
