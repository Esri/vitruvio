#include "PRTLog.h"
#include "Misc/MessageDialog.h"
#include "prtx/Exception.h"
#include "Windows/WindowsPlatformMisc.h"

DEFINE_LOG_CATEGORY(LogEpic);

#pragma region Logging

void FPRTLog::Message(FString TheMessage, ELogVerbosity::Type Type)
{
	switch(Type)
	{
	case ELogVerbosity::NoLogging: break;
	case ELogVerbosity::Fatal:
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("%s\n"), *TheMessage);
		break;
	case ELogVerbosity::Error:
		UE_LOG(LogEpic, Error, TEXT("%s"), *TheMessage);
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("%s\n"), *TheMessage);
		break;
	case ELogVerbosity::Warning:
		UE_LOG(LogEpic, Warning, TEXT("%s"), *TheMessage);
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("%s\n"), *TheMessage);
		break;
	case ELogVerbosity::Display:
		UE_LOG(LogEpic, Display, TEXT("%s"), *TheMessage);
		break;
	case ELogVerbosity::Log:
		UE_LOG(LogEpic, Log, TEXT("%s"), *TheMessage);
		break;
	case ELogVerbosity::Verbose:
		UE_LOG(LogEpic, Verbose, TEXT("%s"), *TheMessage);
		break;
	case ELogVerbosity::VeryVerbose:
		UE_LOG(LogEpic, VeryVerbose, TEXT("%s"), *TheMessage);
		break;
	case ELogVerbosity::NumVerbosity: break;
	case ELogVerbosity::VerbosityMask: break;
	case ELogVerbosity::SetColor: break;
	case ELogVerbosity::BreakOnLog: break;
	default: ;
	}
}


/**
 * \brief Send a log message to the Output window
 * \param Type 
 * \param TheMessage 
 * \param Parameter 
 */
void FPRTLog::Message(const FString TheMessage, const FString Parameter, ELogVerbosity::Type Type)
{
#if WITH_EDITOR
	Message(TheMessage + Parameter, Type);
#endif
}

/**
 * \brief Send a log message to the Output window
 * \param MessageType 
 * \param TheMessage 
 * \param Parameter 
 */
void FPRTLog::Message(const FString TheMessage, const int32 Parameter, ELogVerbosity::Type Type)
{
	//const FString AMessage = TheMessage + FString::FromInt(Parameter);
	Message(TheMessage + FString::FromInt(Parameter), Type);
}

/**
 * \brief Send a log message to the Output window
 * \param Type 
 * \param TheMessage 
 * \param Parameter 
 */
void FPRTLog::Message(const FString TheMessage, const double Parameter, ELogVerbosity::Type Type)
{
	const FString AMessage = TheMessage + FString::Printf(TEXT("%f"), static_cast<float>(Parameter));
	Message(AMessage, Type);
}


/**
 * \brief Send a log message to the Output window
 * \param MessageType 
 * \param TheMessage 
 * \param Parameter 
 */
void FPRTLog::Message(const FString TheMessage, const prt::Status Parameter, ELogVerbosity::Type Type)
{
	const FString AMessage = TheMessage + prt::getStatusDescription(Parameter);
	Message(AMessage, Type);
}

/**
 * \brief Pop-up a message dialog box to the user
 * \param InBody
 * \param InTitle
 */
void FPRTLog::FDialog::Box(FString InBody, FString InTitle)
{
	Body = FText::FromString(InBody);
	Title = FText::FromString(InTitle);
	//we write a log message to make sure
	FMessageDialog::Open(EAppMsgType::Ok, EAppReturnType::Continue, Body, &Title);
}

#pragma endregion


void FPRTLog::WriteContentToDisk(FString FileName)
{
#ifdef DEBUG_MSG
	StartupLogger();
	SetCurrentWorkingDirectoryToPlugin();

	cout << "FPRTLogPluginModule::writeContentToDisk" << endl;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	ofstream ContentWriter;
	const FString ContentDir = L"./Content/" + FileName;

#ifdef DEBUG_MSG
	wcout << "using content directory: " << endl << ContentDir << endl;
#endif

	if (!PlatformFile.DirectoryExists(ContentDir.c_str()))
	{
		wcout << "Attempting to create path... ";
		if (PlatformFile.CreateDirectory(ContentDir.c_str()))
		{
			wcout << "Success" << endl;
		}
		else
		{
			wcout << "Failure" << endl;
		}
	}
	wcout << "Path exists, now attempting to write files." << endl;
	for (auto& i : ObjectFiles)
	{
		FString File = ContentDir + L"/" + i.Value;
#ifdef DEBUG_MSG
		wcout << File << endl;
#endif
		ContentWriter.open(File.c_str(), std::ofstream::binary);
		ContentWriter.write(i.Value.c_str(), i.Value.length());
		ContentWriter.close();
	}
	for (auto& i : Materials)
	{
		FString File = ContentDir + L"/" + i.Value;
		wcout << File << endl;
		ContentWriter.open(File.c_str(), std::ofstream::binary);
		ContentWriter.write(i.Value.c_str(), i.Value.length());
		ContentWriter.close();
	}
	for (auto& i : JpegFiles)
	{
		FString File = ContentDir + L"/" + i.Value;
		wcout << File << endl;
		ContentWriter.open(File.c_str(), std::ofstream::binary);
		ContentWriter.write(reinterpret_cast<char*>(i.Value), JpegSizes[i.Value]);
		ContentWriter.close();
	}
	RestoreOriginalWorkingDirectory();
	ShutDownLogger();
#endif
}

