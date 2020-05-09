#pragma once

#include "Modules/ModuleManager.h"

class UnrealGeometryEncoderModule : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;
};
