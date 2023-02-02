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

#include "UnrealLogHandler.h"

DEFINE_LOG_CATEGORY(UnrealPrtLog)

TArray<FLogMessage> UnrealLogHandler::PopMessages()
{
	FScopeLock Lock(&MessageLock);
	TArray<FLogMessage> Result(Messages);
	Messages.Empty();
	return Result;
}

void UnrealLogHandler::handleLogEvent(const wchar_t* msg, prt::LogLevel level)
{
	FScopeLock Lock(&MessageLock);
	const FString MessageString(WCHAR_TO_TCHAR(msg));
	Messages.Push({MessageString, level});

	switch (level)
	{
	case prt::LOG_TRACE:
		UE_LOG(UnrealPrtLog, VeryVerbose, TEXT("%s"), msg) break;
	case prt::LOG_DEBUG:
		UE_LOG(UnrealPrtLog, Verbose, TEXT("%s"), msg) break;
	case prt::LOG_INFO:
		UE_LOG(UnrealPrtLog, Display, TEXT("%s"), msg) break;
	case prt::LOG_WARNING:
		UE_LOG(UnrealPrtLog, Warning, TEXT("%s"), msg) break;
	case prt::LOG_ERROR:
		UE_LOG(UnrealPrtLog, Error, TEXT("%s"), msg) break;
	case prt::LOG_FATAL:
		UE_LOG(UnrealPrtLog, Fatal, TEXT("%s"), msg) break;
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
