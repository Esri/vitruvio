// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "UnrealLogHandler.h"

DEFINE_LOG_CATEGORY(UnrealPrtLog)

void UnrealLogHandler::handleLogEvent(const wchar_t* msg, prt::LogLevel level)
{
	switch (level)
	{
	case prt::LOG_TRACE:
		UE_LOG(UnrealPrtLog, VeryVerbose, L"%s", msg) break;
	case prt::LOG_DEBUG:
		UE_LOG(UnrealPrtLog, Verbose, L"%s", msg) break;
	case prt::LOG_INFO:
		UE_LOG(UnrealPrtLog, Display, L"%s", msg) break;
	case prt::LOG_WARNING:
		UE_LOG(UnrealPrtLog, Warning, L"%s", msg) break;
	case prt::LOG_ERROR:
		UE_LOG(UnrealPrtLog, Error, L"%s", msg) break;
	case prt::LOG_FATAL:
		UE_LOG(UnrealPrtLog, Fatal, L"%s", msg) break;
	case prt::LOG_NO:
		break;
	default:
		break;
	}
}

const prt::LogLevel* UnrealLogHandler::getLevels(size_t* count)
{
	static prt::LogLevel ALL_LEVELS[ALL_COUNT] = {prt::LOG_FATAL, prt::LOG_ERROR, prt::LOG_WARNING, prt::LOG_INFO, prt::LOG_DEBUG, prt::LOG_TRACE};
	*count = 6;
	return ALL_LEVELS;
}

void UnrealLogHandler::getFormat(bool* dateTime, bool* level)
{
	*dateTime = true;
	*level = true;
}
