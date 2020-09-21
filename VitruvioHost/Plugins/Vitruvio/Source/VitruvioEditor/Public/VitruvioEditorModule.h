// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"

class VitruvioEditorModule final : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;

private:
	FDelegateHandle LevelViewportContextMenuVitruvioExtenderDelegateHandle;
};
