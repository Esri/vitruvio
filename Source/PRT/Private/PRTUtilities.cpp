#include "PRTUtilities.h"
#include "Misc/DateTime.h"
#include "Interfaces/IPluginManager.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinWindows.h"
#include <processenv.h>
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Windows/HideWindowsPlatformTypes.h"

//Current Working Directory management methods:
FString FPRTUtilities::GOriginalWorkingDirectory; //the original Working directory to return to.
FString FPRTUtilities::GPluginDirectory; //the directory the plugin is in, updated only when SetCurrentWorkingDirectoryToPlugin is called.

TArray<FString> FPRTUtilities::SplitString(FString String, const char FindCharacter)
{
	TArray<FString> Output;

	int32 Split = 0;
	while (String.FindChar(FindCharacter, Split))
	{
		FString Out = String.Left(Split);
		String = String.RightChop(Split + 1);
		Output.Add(Out);
	}
	Output.Add(String);
	return Output;
}

double FPRTUtilities::ParseNumber(FString String)
{
	return FCString::Atod(*String);
}

double FPRTUtilities::ParseNumber(const wchar_t* CString, const size_t Size)
{
	const FString String(Size, CString);
	return FCString::Atod(*String);
}

bool FPRTUtilities::StringCompare(const wchar_t* CStringA, const wchar_t* CStringB)
{
	//max size of 1024. Anything larger will not match.
	for (int32 i = 0; i < 1024; i++)
	{
		if (CStringA[i] == 32 && CStringB[i] == 0) return true;

		if (CStringA[i] == 0 && CStringB[i] == 32) return true;

		if (CStringA[i] == 32 && CStringB[i] == 32) return true;

		if (CStringA[i] == 0 && CStringB[i] == 0) return true;

		if (CStringA[i] != CStringB[i]) return false;

		//if it finds a a:'\0' or a:' ' or b:'\0' and it's not aligned, they don't match.
		if (CStringA[i] == ' ' || CStringA[i] == 0 || CStringB[i] == 0) return false;
	}
	return false;
}


#pragma region WorkingDirectory

auto FPRTUtilities::GetAbsolutePath()->FString
{
	return IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(
		*FPaths::ProjectContentDir());
}

//************************************
// Method:    SetCurrentWorkingDirectoryToPlugin
// FullName:  FPRTPluginModule::SetCurrentWorkingDirectoryToPlugin
// Access:    private 
// Returns:   auto
// Qualifier: -> void
//************************************
auto FPRTUtilities::SetCurrentWorkingDirectoryToPlugin() -> FString
{
	GOriginalWorkingDirectory = GetWorkingDirectory();

	const FString FCurrentWorkingDirectory = GetPluginBaseDirectory(); // IPluginManager::Get().FindPlugin("PRT")->GetBaseDir();
	const FString Cwd(TCHAR_TO_UTF8(*FCurrentWorkingDirectory));
	SetCurrentDirectoryA(TCHAR_TO_ANSI(*Cwd));

	// Log.Message(TEXT(">> Setting Current Working Directory to: %s"), *WorkingDirectory());
	return GPluginDirectory = GetWorkingDirectory();
}


//************************************
// Method:    RestoreOriginalWorkingDirectory
// FullName:  FPRTPluginModule::RestoreOriginalWorkingDirectory
// Access:    private 
// Returns:   auto
// Qualifier: const -> void
//************************************
FString FPRTUtilities::RestoreOriginalWorkingDirectory()
{
	SetCurrentDirectoryA(TCHAR_TO_ANSI(*GOriginalWorkingDirectory));
	return GOriginalWorkingDirectory;
}


//************************************
// Method:    GetWorkingDirectory
// FullName:  FPRTPluginModule::GetWorkingDirectory
// Access:    private static 
// Returns:   auto
// Qualifier: -> FString
//************************************
auto FPRTUtilities::GetWorkingDirectory() -> FString
{
	char Buf[256];
	GetCurrentDirectoryA(256, Buf);
	return FString(Buf) + '\\';
}

#pragma endregion


auto FPRTUtilities::GetPluginBaseDirectory() -> FString
{
	return IPluginManager::Get().FindPlugin(PLUGIN_NAME)->GetBaseDir();
}

#pragma region Time

/**
 * \brief Returns the current time in milliseconds (Minutes/Seconds/Milliseconds only)
 * \return double value of NowTime
 */
double FPRTUtilities::GetNowTime()
{
	int32 Year, Month, DayOfWeek, Day, Hour, Minute, Second, Millisecond;
	FPlatformTime::SystemTime(Year, Month, DayOfWeek, Day, Hour, Minute, Second, Millisecond);
	return Hour * 3600 + Minute * 60 + Second + static_cast<double>(Millisecond) / static_cast<double>(1000);
}

/**
 * \brief Calculates elapsed time in seconds from StartTime
 * \param StartTime
 * \return
 */
double FPRTUtilities::GetElapsedTime(const double StartTime)
{
	return (GetNowTime() - StartTime);
}

/**
 * \brief Calculates elapsed time in milliseconds from Start
 * \return
 */
void FPRTUtilities::StartElapsedTimer()
{
	TimerStartTime = GetNowTime();
}

double FPRTUtilities::GetElapsedTime() const
{
	return GetElapsedTime(TimerStartTime);
}

float FPRTUtilities::GetElapsedFloatTime(const double StartTime)
{
	const auto NowTime = GetNowTime();
	return static_cast<float>(NowTime - StartTime) / static_cast<double>(1000);
}


/**
 * \brief
 * \return
 */
int32 FPRTUtilities::GetNowSeconds()
{
	int32 Year, Month, DayOfWeek, Day, Hour, Minute, Second, Millisecond;
	FPlatformTime::SystemTime(Year, Month, DayOfWeek, Day, Hour, Minute, Second, Millisecond);
	return Second;
}


#pragma endregion 
