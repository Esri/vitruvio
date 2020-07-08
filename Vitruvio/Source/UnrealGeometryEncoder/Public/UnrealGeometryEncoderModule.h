// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class UnrealGeometryEncoderModule final : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;
};
