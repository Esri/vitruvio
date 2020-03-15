// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class UnrealGeometryEncoderModule : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;
};
