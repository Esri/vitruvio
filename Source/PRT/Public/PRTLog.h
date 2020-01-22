// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <CoreMinimal.h>
#include "Logging/LogVerbosity.h"
#include "prt/Status.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEpic, All, All);

class  FString;

/**
 * \brief Logging Functions
 */
class FPRTLog
{
public:

	// Message dialog box and printing information to the user.
	struct FDialog
	{
		FText Body;
		FText Title;
		void Box(FString Body, FString Title);
	} Dialog;

	static void Message(FString TheMessage, ELogVerbosity::Type Type = ELogVerbosity::Display);
	static void Message(FString TheMessage, FString Parameter, ELogVerbosity::Type Type = ELogVerbosity::Display);
	static void Message(FString TheMessage, int32 Parameter, ELogVerbosity::Type Type = ELogVerbosity::Display);
	static void Message(FString TheMessage, double Parameter, ELogVerbosity::Type Type = ELogVerbosity::Display);
	static void Message(FString TheMessage, prt::Status Parameter, ELogVerbosity::Type Type = ELogVerbosity::Display);
	
	static void WriteContentToDisk(FString FileName);
};