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

#include "Core.h"
#include "prt/LogHandler.h"

DECLARE_LOG_CATEGORY_EXTERN(UnrealPrtLog, Log, All);

struct FLogMessage
{
	FString Message;
	prt::LogLevel Level;
};

class UnrealLogHandler final : public prt::LogHandler
{

	TArray<FLogMessage> Messages;
	FCriticalSection MessageLock;

public:
	UnrealLogHandler() = default;

	virtual ~UnrealLogHandler() = default;

	TArray<FLogMessage> PopMessages();

	void handleLogEvent(const wchar_t* msg, prt::LogLevel level) override;
	const prt::LogLevel* getLevels(size_t* count) override;
	void getFormat(bool* dateTime, bool* level) override;
};
