// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "Core.h"
#include "prt/LogHandler.h"

DECLARE_LOG_CATEGORY_EXTERN(UnrealPrtLog, Log, All);

class UnrealLogHandler : public prt::LogHandler
{
public:
	UnrealLogHandler() = default;

	virtual ~UnrealLogHandler() = default;

	void handleLogEvent(const wchar_t* msg, prt::LogLevel level) override;
	const prt::LogLevel* getLevels(size_t* count) override;
	void getFormat(bool* dateTime, bool* level) override;
};
