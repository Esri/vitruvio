// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "PRTActor.h"
#include "PRTModule.h"
#include "PRTUtilities.h"
#include "PRTLog.h"

#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include <CoreMinimal.h>


/**
 * \brief The State Machine. Runs all the time and controls Generation.
 */

// Forward Declarations
class APRTActor;
class FPRTModule;
class FPRTLog;
struct FPRTMeshStruct;
struct FPRTAttribute;

class FGenerator : public FRunnable
{
public:
	static FGenerator* PRTGenerator;

	/**
	 * \brief Constructor 
	 * \param InPRTActor - Ref Pointer to the parent object
	 * \param InPRTModule
	 */
	FGenerator(APRTActor* InPRTActor, FPRTModule* InPRTModule); // , TArray<FPRTMeshStruct>& InMeshStruct, FPRTMeshStruct& InMaterialMesh, TMap<FString, FPRTAttribute> InAttributes);
	~FGenerator();
	void SetGenerateState(bool NewState);

	void StartStateManagerThread();
	void Generate();

	/**
	 * \brief Checks to see if the Generate process is completed
	 * \return
	 */
	bool IsFinished() const;

private:
	FRunnableThread* StateManagerRunnableThread;

	FThreadSafeCounter StopTaskCounter;
	FCriticalSection ThreadMutex;
	FEvent* ThreadSemaphore;
	FThreadSafeBool bPauseThread;
	FThreadSafeBool bIsRunning;
	
	// FRunnable functions
	bool Init() override;
	uint32 Run() override;
	void Stop() override; // Covered in FPRTRunnable
	void Exit() override;
	
public:
	void EnsureCompletion();
	void Shutdown();
	void PauseThread();
	bool IsThreadPaused();
	void ContinueThread();
	bool IsRunning();
	
	//void SetState(EStateManagerState NewState);
	prt::Status GenerateStatusCode;
	
private:
	/** The Actor Dependency Injection **/
	APRTActor* PRTActor;
	FPRTModule* PRTModule;

	double ProcessStartTime;
	double LastProcessTime;
	
protected:

	// Utilities
	FPRTUtilities PRTUtil;
	FPRTLog PRTLog;

	float DeltaTime, LastTime;
	const float DefaultDelay = 0.05f;
	
	FThreadSafeBool bPRTGenerating;
	FThreadSafeBool bPRTDataReady;
	FThreadSafeBool StateManagerInitialized;
};
