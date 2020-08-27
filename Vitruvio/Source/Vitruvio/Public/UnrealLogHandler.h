// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "Core.h"
#include "prt/LogHandler.h"

DECLARE_LOG_CATEGORY_EXTERN(UnrealPrtLog, Log, All);

class UnrealLogHandler final : public prt::LogHandler
{
public:
	UnrealLogHandler() = default;

	virtual ~UnrealLogHandler() = default;

	void handleLogEvent(const wchar_t* msg, prt::LogLevel level) override;
	const prt::LogLevel* getLevels(size_t* count) override;
	void getFormat(bool* dateTime, bool* level) override;
};
