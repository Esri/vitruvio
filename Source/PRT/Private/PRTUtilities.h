// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <codecvt>
#include <CoreMinimal.h>

class FString;

#define PLUGIN_NAME "PRT"

class FPRTUtilities
{
public:

	static double ParseNumber(FString String);
	static double ParseNumber(const wchar_t* CString, size_t Size);
	static bool StringCompare(const wchar_t* CStringA, const wchar_t* CStringB);
	auto GetAbsolutePath() -> FString;

	// Compares a and b and tests if same, built specifically and only for obj parsing.
	static TArray<FString> SplitString(FString String, char FindCharacter);

	// Used to convert strings to wstrings (Unicode)
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> StringConverter;

	static FString SetCurrentWorkingDirectoryToPlugin(); //set it to the active plugin directory.
	static FString RestoreOriginalWorkingDirectory(); //return it to the original path for Unreal Engine.

	static FString GetWorkingDirectory(); //retrieve the current working directory.
	static FString GetPluginBaseDirectory();

	static double GetNowTime();
	static double GetElapsedTime(double StartTime);
	static float GetElapsedFloatTime(double StartTime);

	//Current Working Directory management methods:
	static FString GOriginalWorkingDirectory; //the original Working directory to return to.
	static FString GPluginDirectory; //the directory the plugin is in, updated only when SetCurrentWorkingDirectoryToPlugin is called.

private:
	void StartElapsedTimer();
	double GetElapsedTime();
	double TimerStartTime = 0;
	
};
