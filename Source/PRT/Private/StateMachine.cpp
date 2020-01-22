// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "StateMachine.h"
//#include "PlatformProcess.h"
#include "HAL/RunnableThread.h"


//FPRTStateMachine::StateMachine = nullptr;

/**
 * \brief Initialize Values and create Worker Thread to hold that thread.
 * \return 
 */
prt::Status FPRTStateMachine::StartGenerateWorkerThread()
{
	PRTActor.bAttributesUpdated = false;
	PRTActor.bGenerate = false;
			
	// Set Timers
	PRTActor.LastGenerationElapsedTime = PRTActor.CurrentGenerationTime;
	PRTActor.IdleTime = PRTActor.CurrentGenerationTime = 0.0f;
	PRTActor.MeshingTime = 0.0f;
	PRTActor.GenerateCount += 1;

	// Set the current building to an XRay material?
	// ....or clear the mesh?
			
	// TODO: This shows the structure in classic plugin.
	// TODO: Create and monitor a Worker Thread that is separate from State Machine

	// From PRTActor:


	const prt::Status GetModelStatus = GenerateModelInPRT();
	if (GetModelStatus != prt::STATUS_OK)
	{
		PRTLog.Message(TEXT(">> Generate failed in PRTModule.GenerateModelInPRT - aborting. Status: "), 
			GetModelStatus, ELogVerbosity::Warning);
		return GetModelStatus;
	}

	//FPRTMeshEncoder MeshEncoder(&PRTPlugin);
	PRTActor.GetModelData(MeshStruct);

	PRTLog.Message(TEXT("> Building Generation Started."));
			

	// TODO: Worker thread to start/run/unpause
	//PRTWorker->Init();
	//PRTWorker->Run();
	PRTActor.State = EPRTTaskState::GENERATING;
	return GetModelStatus;
}

/**
 * \brief Separate Thread to manage machine state
 */
uint32 FPRTStateMachine::Run()
{
	double LastTime = 0;
	prt::Status Result = prt::STATUS_OK;
	PRTActor.State = EPRTTaskState::IDLE;
	
	// Short wait before starting
	FPlatformProcess::Sleep(0.03);

	while (StopTaskCounter.GetValue() == 0) // && !IsFinished())
	{
		// Calculate the DeltaTime since the last loop
		//DeltaTime = PRTUtil.GetElapsedTime(LastTime);
		//LastTime = PRTUtil.GetNowTime();
	
		// Determine what to do next based on the Machine State
		//
		switch (PRTActor.State)
		{
#pragma region Idle
		case EPRTTaskState::IDLE:
			// TODO: 
			// Waiting for Generate, increment DeltaTime
			PRTActor.IdleTime += DeltaTime;

			// bGenerate is used by the BP to signal a Generate is ready.
			if (PRTActor.bGenerate)
			{
				PRTActor.State = EPRTTaskState::IDLETOGEN;
			}
			else
			{
				break;
			}
#pragma endregion Idle
			
#pragma region Idle to Generate
		case EPRTTaskState::IDLETOGEN:
			// TODO:
			// Entry when bGenerate == True during Idle state

			// Currently, Generate is called too often. 
			// Is there a minimum time we should wait between generates?
			// Should we loop to another tick to ensure RPK data is stable for a minimum time?
			// Do we queue and wait to see if data stops updating?
			if (PRTActor.IdleTime < 0.2)
			{
				// Not enough time, so loop until minimum time is met.
				PRTActor.State = EPRTTaskState::IDLE;
				//return;
			}
			
			// TODO: Handle if bAttributesDirty is not set in non-editor mode. If this is a thing.
			// Check to see if the data has changed. If No Editor, perhaps always Gen, but if editor only when bIsDirty
			if (PRTActor.bHasEditor && !PRTActor.bAttributesUpdated)
			{
				// Generate is called too often, even when data hasn't changed. Ignore if no changes.
				PRTActor.bGenerate = false;

				PRTActor.State = EPRTTaskState::IDLE;
			}
			else
			{
				Result = StartGenerateWorkerThread();
				if (Result != prt::STATUS_OK)
				{
					PRTLog.Message(TEXT("Start Worker Thread error in FPRTStateMachine::Run(). Status: "), Result, ELogVerbosity::Warning);
				}
			}
			break;
#pragma endregion

#pragma region Generating
		case EPRTTaskState::GENERATING:
			PRTActor.CurrentGenerationTime += DeltaTime;

			// TODO: Look for error from the Generation worker thread and handle.
			
			// TODO: Manage any errors or change of state during generation
			
			// TODO: Manage the canceling of Generate

			// TODO: Manage 'IsDone' from Worker Thread

			break;
#pragma endregion

#pragma region Generate to Mesh
		case EPRTTaskState::GENTOMESH:
			// Data is ready to mesh
			// Could transfer to TArray for BP, or mesh here. Separate thread, or let worker thread handle it? Just status in that case.
			//
			PRTActor.bGenerate = false;
			PRTActor.LastGenerationElapsedTime = PRTActor.CurrentGenerationTime;
			PRTActor.MeshingTime = 0.0f;

			// TODO: Move Create Mesh into the Generate Thread
			
			break;

#pragma endregion
			
#pragma region Meshing
		case EPRTTaskState::MESHING:
			PRTActor.MeshingTime += DeltaTime;

			PRTActor.State = EPRTTaskState::MESHINGTOCOMPL;
			break;
#pragma endregion
			
		case EPRTTaskState::MESHINGTOCOMPL:
			// Likely NoOp
			PRTActor.State = EPRTTaskState::COMPLETE;
			break;

		case EPRTTaskState::COMPLETE:
			// Any cleanup tasks
			// Pause Worker PRTWorkerRunnableThread?
			// 
			PRTActor.State = EPRTTaskState::IDLE;
			
			break;

		default:
			PRTActor.State = EPRTTaskState::IDLE;
			break;
		}

		FPlatformProcess::Sleep(0.050);
	}
	return 0;
}


FPRTStateMachine::FPRTStateMachine(APRTActor& InPRTActor, TArray<FPRTMeshStruct>& InMeshStruct, 
	FPRTMeshStruct& InMaterialMesh, TMap<FString, FPRTAttribute> InAttributes)
	: MeshStruct(InMeshStruct), MaterialMesh(InMaterialMesh),
		Attributes(InAttributes), PRTActor(InPRTActor),
	StopTaskCounter(0)
{
	// Initialize the PRT Plugin

	// Start the State Machine
	RunnableThread = FRunnableThread::Create(this, TEXT("FPRTStateMachine"), 0, TPri_Normal);

	auto RunnableStateMachine = new FPRTStateMachine(PRTActor, MeshStruct, MaterialMesh, Attributes);
	RunnableStateMachine->Init();
	RunnableStateMachine->Run();
}

FPRTStateMachine::~FPRTStateMachine()
{
	StateMachine->Stop();
	delete RunnableThread;
	RunnableThread = nullptr;
}


int32 FPRTStateMachine::Generate()
{

	return prt::STATUS_OK;
}

prt::Status FPRTStateMachine::GenerateModelInPRT()
{
	return {};
}

void FPRTStateMachine::SetRPKFile(const TCHAR* RpkPath)
{

}

void FPRTStateMachine::SetInitialShape(const TCHAR* OBJPath)
{

}

auto FPRTStateMachine::GetAttributes() -> TMap<FString, FPRTAttribute>
{
	return {};
}


TArray<FPRTMeshStruct> GMeshStruct;

/**
 * \brief Called from PRTActor. Fire-and-forget Generate Request
 * \param InMeshStruct 
 * \param InAttributes 
 */
void FPRTStateMachine::GenerateModel(TArray<FPRTMeshStruct>& InMeshStruct, TMap<FString, FPRTAttribute> InAttributes)
{
	GMeshStruct = InMeshStruct;
	FPRTModule::Attributes = InAttributes;
	
}

void FPRTStateMachine::SetMeshStruct(TArray<FPRTMeshStruct>& InMeshStruct)
{
}

#pragma region Runnable

auto FPRTStateMachine::StateMachineRunnableThread() -> void
{
}

auto FPRTStateMachine::InitRunnableThread(APRTActor* InActor) -> FPRTStateMachine*
{
	return nullptr;
}

bool FPRTStateMachine::Init()
{
	// Create and Run a PRT Worker Instance
	//PRTWorker = new FPRTWorker(PRTPlugin, MeshStruct, &PRTActor);
	//PRTWorker->Init();
	//PRTWorker->Run();
	
	return false;
}

void FPRTStateMachine::Stop()
{
	StopTaskCounter.Increment();
}

void FPRTStateMachine::Exit()
{
	//throw std::logic_error("The method or operation is not implemented.");
}

class FSingleThreadRunnable* FPRTStateMachine::GetSingleThreadInterface()
{
	throw std::logic_error("The method or operation is not implemented.");
}

#pragma endregion 

/**
 * \brief Finished if Complete or Idle state.
 * \return 
 */
auto FPRTStateMachine::IsFinished() -> bool
{
	const auto bFinished = (PRTActor.State == EPRTTaskState::COMPLETE) ||
		(PRTActor.State == EPRTTaskState::IDLE);

	return bFinished;
}

/**
 * \brief  Checks to see if the PRT Plugin is loaded
 * \return PRT Plugin Status
 */
bool FPRTStateMachine::IsLoaded()
{
	// TODO: 
	return false;
}

