// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "PRTGenerator.h"
//#include "PlatformProcess.h"
#include "HAL/RunnableThread.h"
#include "PRTActor.h"

// For the Linker.
FGenerator* FGenerator::PRTGenerator = nullptr; // FRunnable

#pragma region Constructor-Destructor
/**
 * \brief
 * \param InPRTActor
 * \param InPRTModule
 */
FGenerator::FGenerator(APRTActor* InPRTActor, FPRTModule* InPRTModule)
{
	// Note that the actor BP should not be set to run the Construction script when translation occurs

	PRTActor = InPRTActor; // Reference to the Actor that is using the State Manager
	PRTModule = InPRTModule;

	bPauseThread = false; // To cause the thread to sleep until restored
	SetGenerateState(false);

}

void FGenerator::StartStateManagerThread()
{
	//if (StateManagerRunnableThread) return;
	if (StateManagerInitialized) return;

	// Create a unique thread name based on the name of the actor using the thread manager
	const FString ThreadName = PRTActor->GetName() + "::FPRTStateManager";

	// Create the FRunnable thread for the State Manager. Loops in the Run() fn.
	StateManagerRunnableThread = FRunnableThread::Create(this, *ThreadName, 0, TPri_BelowNormal);

	ThreadSemaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	if (StateManagerRunnableThread)
	{
		PRTLog.Message(TEXT("State Manager Thread Started."));
		StateManagerInitialized = true;
		PRTActor->StateManagerRuntime = 0.0f;
	}
	else
	{
		PRTLog.Message(TEXT("State Manager Thread Not Running."));
	}
}

void FGenerator::Generate()
{
	if (bPRTGenerating) return;

	bPRTDataReady = false;
	SetGenerateState(true);
}

FGenerator::~FGenerator()
{
	PRTGenerator->Stop();

	delete StateManagerRunnableThread;
	StateManagerRunnableThread = nullptr;
}

#pragma endregion 

#pragma region FRunnable Fns
void FGenerator::SetGenerateState(bool NewState)
{
	bPRTGenerating = NewState;
	PRTActor->bGenerating = NewState;
}

/**
 * \brief Separate Thread to manage machine state
 */
uint32 FGenerator::Run()
{
	prt::Status Result = prt::STATUS_OK;
	
	// Short wait before starting
	FPlatformProcess::Sleep(DefaultDelay);
	bIsRunning = true;

	while (StopTaskCounter.GetValue() == 0) // && !IsFinished())
	{
		// Calculate the DeltaTime since the last loop
		DeltaTime = PRTUtil.GetElapsedTime(LastTime);
		PRTActor->GenerateIdleTime += DeltaTime;
		LastTime = PRTUtil.GetNowTime();
		
		PRTActor->StateManagerRuntime += DeltaTime;	// Temp for testing thread running state
		
		if (bPauseThread)
		{
			//FEvent->Wait(); will "sleep" the thread until it will get a signal "Trigger()"
			PRTLog.Message(TEXT("SM Thread Paused."));
			ThreadSemaphore->Wait();
		}
		
		if(bPRTGenerating && !bPRTDataReady)
		{
			// Thread is blocked until complete.
			PRTLog.Message(TEXT("Generate Started."));
			PRTUtil.StartElapsedTimer();
			const auto Status = PRTModule->GenerateModel();
			
			PRTActor->LastGenerationElapsedTime = static_cast<float>(PRTUtil.GetElapsedTime());

			if (Status != prt::STATUS_OK)
			{
				PRTLog.Message(
					TEXT(">> Generate failed in FGenerator::Run >> PRT.GenerateModel - aborting. Status: "),
					Status, ELogVerbosity::Warning);
			}
			else
			{
				bPRTDataReady = true;
			}

			PRTActor->GenerateIdleTime = 0.0f;

			PRTLog.Message(TEXT(">> Generate complete, elapsed time (ms): "), PRTUtil.GetElapsedTime());
			SetGenerateState(false);
			FPlatformProcess::Sleep(DefaultDelay);
			
			PRTActor->GenerateCompleted(bPRTDataReady);
		}

		FPlatformProcess::Sleep(DefaultDelay);
	}

	bIsRunning = false;
	return 0;
}

/**
 * \brief Check if the FRunnable Run() function is active.
 * \return bIsRunning private member
 */
bool FGenerator::IsRunning()
{
	return bIsRunning;
}

void FGenerator::Stop()
{
	StopTaskCounter.Increment();
}

void FGenerator::Exit()
{
	
}

void FGenerator::EnsureCompletion()
{
	Stop();

	if (StateManagerRunnableThread)
	{
		StateManagerRunnableThread->WaitForCompletion();
	}
}

void FGenerator::Shutdown()
{
	StopTaskCounter.Increment();
	ContinueThread();	// Make sure it is not paused
}

/**
 * \brief Sets bThreadPaused. Run() handles actual Wait
 */
void FGenerator::PauseThread()
{
	// Run() handles the pause
	bPauseThread = true;
}

/**
 * \brief Returns whether the thread is paused
 * \return bPauseThread
 */
bool FGenerator::IsThreadPaused() 
{
	return bool(bPauseThread);
}

/**
 * \brief Un-Pause the thread if paused
 */
void FGenerator::ContinueThread()
{
	if (!bPauseThread) return;

	PRTLog.Message(TEXT("SM Thread Un-Paused."));
	
	bPauseThread = false;

	if (ThreadSemaphore)
	{
		// Trigger the event to wake up the thread
		ThreadSemaphore->Trigger();
	}
}


#pragma endregion



#pragma region Runnable


/**
 * \brief INIT State and create the Importer FRunnable process.
 * \return true
 */
bool FGenerator::Init()
{
	return true;
}

#pragma endregion 

/**
 * \brief Finished if Complete or IDLE state.
 * \return 
 */
auto FGenerator::IsFinished() const -> bool
{
	return bPRTGenerating;
}

