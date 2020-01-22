// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "PRTActor.h"
//#include "IProceduralRuntimeModule.h"
/// #include "MeshEncoder.h"

#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
//#include "PRTWorker.h"


class FPRTStateMachine;
class FPRTWorker;
class FPRTPluginModule;
//// class FPRTMeshEncoder;
class FPRTLog;
struct FPRTMeshStruct;
struct FPRTAttribute;

/**
 * \brief The State Machine. Runs all the time and controls Generation.
 */

class IPRTStateMachine : public FRunnable
{
	// Called from PRTActor.
// Fire-and-forget Generate Request
// The address of MeshStruct should already be injected
	virtual void GenerateModel(TArray<FPRTMeshStruct>& InMeshStruct,
		TMap<FString, FPRTAttribute> InAttributes) = 0;

	virtual void SetMeshStruct(TArray<FPRTMeshStruct>& InMeshStruct) = 0;

	// Step 1
	virtual void SetRPKFile(const TCHAR* RpkPath) = 0;
	virtual void SetInitialShape(const TCHAR* OBJPath) = 0;

	// Step 2: Get Rules

	// Step 3:
	virtual auto GetAttributes()->TMap<FString, FPRTAttribute> = 0;
	// initializer_list<TPairInitializer<const FString&, const FPRTAttribute&>> GetAttributes();
	
	/**
	 * \brief Step 4: Create Building
	 * \return Generate Status
	 */
	virtual int32 Generate() = 0;
	virtual auto GenerateModelInPRT()->prt::Status = 0;

	/**
	 * \brief Checks to see if the Generate process is completed
	 * \return
	 */
	virtual bool IsFinished() = 0;
	virtual bool IsLoaded() = 0;

	/**
	* \brief Set to True to cause the Generate process to begin
	 * Is set to false and
	 */
	bool bGenerate = false;
	
};

class FPRTStateMachine : public IPRTStateMachine
{
public:
	/**
	 * \brief Constructor 
	 * \param InPRTActor - Ref Pointer to the parent object
	 * \param InMeshStruct - The MeshStruct reference sent from the BP_RPKDecoder
	 * \param InMaterialMesh - Ref to MaterialMesh in Actor
	 * \param InAttributes - Ref to Attributes in Actor
	 */
	FPRTStateMachine(APRTActor& InPRTActor, TArray<FPRTMeshStruct>& InMeshStruct, FPRTMeshStruct& InMaterialMesh, TMap<FString, FPRTAttribute> InAttributes);
	~FPRTStateMachine();

	// Called from PRTActor.
	// Fire-and-forget Generate Request
	// The address of MeshStruct should already be injected
	void GenerateModel(TArray<FPRTMeshStruct>& InMeshStruct, TMap<FString, FPRTAttribute> InAttributes) override;

	void SetMeshStruct(TArray<FPRTMeshStruct>& InMeshStruct) override;
	

	// Step 1
	void SetRPKFile(const TCHAR* RpkPath) override;
	void SetInitialShape(const TCHAR* OBJPath) override;

	// Step 2: Get Rules

	// Step 3:
	auto GetAttributes() -> TMap<FString, FPRTAttribute> override;
	// initializer_list<TPairInitializer<const FString&, const FPRTAttribute&>> GetAttributes();

	/**
	 * \brief Step 4: Create Building
	 * \return Generate Status
	 */
	int32 Generate() override;
	auto GenerateModelInPRT() -> prt::Status override;

	/**
	 * \brief Checks to see if the Generate process is completed
	 * \return
	 */
	bool IsFinished() override;

	bool IsLoaded() override;

	/**
	* \brief Set to True to cause the Generate process to begin
	 * Is set to false and
	 */
	//bool bGenerate = false;

private:

	/**
	* \brief The Mesh Structure the Generate process populates. From PRTActor
	 */
	TArray<FPRTMeshStruct>& MeshStruct;
	FPRTMeshStruct& MaterialMesh;
	TMap<FString, FPRTAttribute> Attributes;

	class FSingleThreadRunnable* GetSingleThreadInterface() override;

	/**
	 * \brief
	 */
	void StateMachineRunnableThread();

	/**
	 * \brief Start the thread and the worker from static fn
	 * This code ensures only one Generate thread will be able to run at a time.
	 * This function returns a handle to the newly started instance.
	 * \param MeshStruct
	 * \param InActor
	 * \return
	 */
	FPRTStateMachine* InitRunnableThread(APRTActor* InActor);

	FPRTStateMachine* StateMachine;
	prt::Status StartGenerateWorkerThread();

	// Interface Virtual Functions
	bool Init() override;

	uint32 Run() override;
	void Stop() override;
	void Exit() override;

	/** The Actor Dependency Injection **/
	APRTActor& PRTActor;
	FPRTWorker* PRTWorker;
	FPRTModule PRTPlugin;
	//FProceduralRuntimeModule PRTModule;


	/** Stop this thread? Uses PRTWorkerRunnableThread Safe Counter */
	FThreadSafeCounter StopTaskCounter;

	/**
	 * \brief Runnable Thread for the State Machine
	 */
	FRunnableThread* RunnableThread;


	/**
	 * \brief For use in PRTWorkerRunnableThread timing
	 */
	float DeltaTime;

	// Internal Status needed?
	// FThreadSafeBool

//	FPRTUtilities PRTUtil;

	FPRTLog PRTLog;

};
