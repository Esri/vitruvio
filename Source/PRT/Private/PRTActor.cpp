// Fill out your copyright notice in the Description page of Project Settings.

#include "PRTActor.h"
//#include "Engine/Engine.h"
#include "HAL/FileManager.h"
#include <Containers/Array.h>
#include <Containers/Map.h>
#include "prt/Status.h"
// #include "StateMachine.h"


// Sets default values
// This happens five times for some reason and class is init from start
APRTActor::APRTActor()
{
#if WITH_EDITOR
	bHasEditor = true;
#else
	bHasEditor = false;
#endif

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//bind the event to the delegate. This should fire the function call within UE4.

	// Elements that are needed in Editor and in-Game, so init here.
	BuildFileLists();
	
	//GetComponents();	// Root is null, doesn't work.
}

void APRTActor::InitializeStateMachine()
{

	
}

void APRTActor::GetComponents()
{
	// The RootComponent is a nullptr, and other methods don't return pointers, either
	
	USceneComponent* TheRoot = GetRootComponent();
	
	if (TheRoot)
	{
		TArray<USceneComponent*> SceneComps;
		TheRoot->GetChildrenComponents(true, SceneComps);
		//GetComponentByClass(UStaticMeshComponent::StaticClass());
		if (!SceneComps.Num() == 0)
		{
			if (SceneComps[0]->GetName() == TEXT("StaticMesh"))
			{
				PRTStaticMesh = Cast<UStaticMeshComponent>(SceneComps[0]);
			}
		}

		TArray<UActorComponent*> ProcMeshComps =
			GetComponentsByClass(UProceduralMeshComponent::StaticClass());
		if (!ProcMeshComps.Num() == 0)
			PRTProceduralMesh = Cast<UProceduralMeshComponent>(ProcMeshComps[0]);

		TArray<UActorComponent*> BoxComps =
			GetComponentsByClass(UBoxComponent::StaticClass());
		if (!BoxComps.Num() == 0)
			PRTCollisionBox = Cast<UBoxComponent>(BoxComps[0]);

		SceneComps.Empty();
		ProcMeshComps.Empty();
		BoxComps.Empty();
	}
}

// Called when the game starts or when spawned
void APRTActor::BeginPlay()
{
	Super::BeginPlay();
	PRTLog.Message(TEXT("APRTActor::BeginPlay()"));

	GenerateCount = 0;
	GenerateSkipCount = 0;
}

// Called every frame
void APRTActor::Tick(float TickDeltaTime)
{
	Super::Tick(TickDeltaTime);

	// Ticks only happen in-game, so the Worker Thread handles State Machine and Generation control

}

#pragma region Files

/**
 * Creates a list of OBJ and RPK files found in the project.
 * Looks recursively at the folders, so they can be placed anywhere.
 */
void APRTActor::BuildFileLists(bool bRescan)
{
	if (bRescan == false && OBJFiles.Num() != 0) return;

	const FString ContentDir = FPaths::ProjectContentDir(); // FPaths::GameContentDir();
	IFileManager& File_Manager = IFileManager::Get();

	GetObjFileList(File_Manager, ContentDir);
	GetRPKFileList(File_Manager, ContentDir);
}

void APRTActor::GetObjFileList(IFileManager& FileManager, FString ContentDir)
{
	TArray<FString> Files;
	int32 Start, Finish;

	OBJFiles.Empty();

	FString FileExtension = "*.obj";
	FileManager.FindFilesRecursive(Files, *ContentDir, *FileExtension, true, false, false);

	PRTLog.Message(TEXT("> Object Files Found: "), Files.Num());

	for (auto i = 0; i < Files.Num(); i++)
	{
		FOBJFile OBJ_File;
		Files[i].FindLastChar('/', Start);
		Files[i].FindLastChar('.', Finish);
		OBJ_File.Name = Files[i].Mid(Start + 1, Finish - Start - 1);
		OBJ_File.Path = Files[i].Mid(ContentDir.Len());
		OBJFiles.Add(OBJ_File);
	}
	Files.Empty();
}

void APRTActor::GetRPKFileList(IFileManager& FileManager, FString ContentDir)
{
	TArray<FString> Files;
	int32 Start, Finish;

	RPKFiles.Empty();

	// Add an Empty option for the initial RPK selection
	FRPKFile RpkFile;
	RpkFile.Name = "(none)";
	RpkFile.Path = "";
	RPKFiles.Add(RpkFile);

	FString FileExtension = "*.rpk";
	FileManager.FindFilesRecursive(Files, *ContentDir, *FileExtension, true, false, false);

	PRTLog.Message(TEXT("> RPK Files Found: "), Files.Num());

	for (auto i = 0; i < Files.Num(); i++)
	{
		Files[i].FindLastChar('/', Start);
		Files[i].FindLastChar('.', Finish);
		RpkFile.Name = Files[i].Mid(Start + 1, Finish - Start - 1);
		RpkFile.Path = Files[i].Mid(ContentDir.Len());
		RPKFiles.Add(RpkFile);
	}
	Files.Empty();
}

#pragma endregion 

#pragma region Generation

/**
 * \brief Initialize the RPKFile in PRTPlugin, and ViewAttributes, and Details panel.
 * \param bCompleteReset 
 */
void APRTActor::InitializeRPKData(bool bCompleteReset)
{
	if(RPKFile == "(none)")
	{
		PRTLog.Message(FString(L">> APRTActor::InitializeRPKData - RPKFile Undefined."), ELogVerbosity::Warning);
		return;
	}

	bAttributesUpdated = true; // Allow it to regenerate	
	auto SetRPKStatus = PRT.SetRPKFile(*RPKPath);

	if(SetRPKStatus != prt::STATUS_OK)
	{
		PRTLog.Message(FString(L">> APRTActor::InitializeRPKData - SetRPKFile Status: "), SetRPKStatus, ELogVerbosity::Warning);
	}

	CopyViewAttributesIntoDataStore();
	if (bCompleteReset)
	{
		EraseAttributes();
	}

	if (PRT.IsLoaded() == true)
	{
		UseFirstRule();

		PRT.SetInitialShape(*OBJPath);
		Attributes = PRT.GetAttributes();

		if (bCompleteReset)
		{
			InitializeViewAttributes();
		}
		RecallViewAttributes();
	}
	else
	{

		PRTLog.Message(FString(L">> APRTActor::InitializeRPKData - PRT Plugin is not loaded."), ELogVerbosity::Warning);
	}

#if WITH_EDITOR
	if (PrtDetail != nullptr)
	{
		// ReSharper disable once CppExpressionWithoutSideEffects
		PrtDetail->Refresh();
		PRTLog.Message(FString(L">> APRTActor::InitializeRPKData Complete."));
	}
	else
	{
		// Likely that you are in PlayInEditor and the Details panel isn't available	
		//PRTLog.Message(FString(L">> APRTActor::InitializeRPKData - PrtDetail == nullptr"), ELogVerbosity::Warning);
	}
#endif
}

/**
 * \brief Allow control of State Machine Loop
 * \param NewStatus 
 */
auto APRTActor::SetRPKState(EPRTTaskState NewStatus) -> void
{
	APRTActor::State = NewStatus;
}

/**
 * \brief Returns true is internal state and the Compare are the same
 * \param CompareStatus 
 * \return True if same
 */
bool APRTActor::CompareRPKState(EPRTTaskState CompareStatus) const
{
	return (APRTActor::State == CompareStatus);
}

/**
 * \brief RPKs only have one @Start rule - use the first Rule found.
 */
void APRTActor::UseFirstRule()
{
	// RPKs only have one @Start rule
	// Use the first Rule found.
	// TODO: Move to PRT and place code where needed.

	TArray<FString> Rules_Out = PRT.GetRules();
	Rules.Empty();
	for (int32 i = 0; i < Rules_Out.Num(); i++)
	{
		Rules.Add(*Rules_Out[i]);
	}

	//setting the first rule found
	const FString Use_Rule = *Rules[0];
	PRT.SetRule(Use_Rule);
}


/**
 * \brief Main fn Call from Blueprints. Modifies input MeshStruct with generated building data.
 * \param MeshStruct
 * \param bForceRegen 
 */
void APRTActor::GenerateModelData(bool bForceRegen)
{
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("GenerateModelData called with %s RPK and %s OBJ\n"), *RPKFile, *OBJFile);

	if (RPKFile == "(none)")
	{
		PRTLog.Message(FString(L">> APRTActor::GenerateModelData - RPKFile Undefined."), ELogVerbosity::Warning);
		State = EPRTTaskState::IDLE;
		bAttributesUpdated = false;
		return;
	}


	if (State == EPRTTaskState::GENERATING && !bForceRegen)
	{
		// Can't call Generate while one is in-process
		// bAttributesUpdated remains set, so another regen will happen, but
		// after this Generate and the minimum delay time.
		return;
	}
	
	const double StartTime = PRTUtil.GetNowTime();
	if ((StartTime - LastGenerationTimestamp) < MinimumTimeBetweenRegens && !bForceRegen)
	{
		PRTLog.Message(TEXT("Generation Interval too short, using Cache Data."), ELogVerbosity::Warning);
		//GetModelData(MeshStruct);
		State = EPRTTaskState::GENTOMESH;	// Allow to just reuse cache data.
		return;
	}

	State = EPRTTaskState::GENERATING;
	// Needed to save initial state as Attribute Copy routines set bool to true
	const auto bTempAttributesUpdated = bAttributesUpdated;	

	// We have an RPK file but the plugin isn't loaded
	if ((PRT.IsLoaded() == false && RPKFile.Len() > 0) || (Attributes.Num() == 0))
	{
		InitializeRPKData(false);
	}
	else
	{
		CopyViewAttributesIntoDataStore();
	}

	// Plugin still won't load, abort
	if (PRT.IsLoaded() == false)
	{
		PRTLog.Message(TEXT("APRTActor::GenerateModelData abort: Plugin is not loaded."), ELogVerbosity::Warning);
		State = EPRTTaskState::IDLE;
		return;
	}

	// Transfer View Attributes to Local Attributes, and then pass to the PRT Module for processing.
	CopyViewAttributesToAttributes();

	if (bTempAttributesUpdated)
	{
		PRT.ApplyAttributesToProceduralRuntime(Attributes);
		
		const prt::Status GetModelStatus = PRT.GenerateModel();
		if (GetModelStatus != prt::STATUS_OK)
		{
			PRTLog.Message(
				TEXT(">> Generate failed in PRT.GenerateModel - aborting. Status: "), GetModelStatus, ELogVerbosity::Warning);
			return;
		}

		ProcessPRTVertexDataIntoMeshStruct(MeshStructureStore);
		
		LastGenerationElapsedTime = PRTUtil.GetElapsedTime(StartTime);
		const FString TheMessage = FString::Printf(TEXT("Generate Count: %d. Elapsed time: %f (s). Array Length: %d."), 
			++GenerateCount, LastGenerationElapsedTime, static_cast<int32>(MeshStructureStore.Num()));
		PRTLog.Message(TheMessage);
		PRTLog.Message(TEXT("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"));

		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Generate Count : %d.Elapsed time : %f(ms).Array Length : %d."), 
			GenerateCount, PRTUtil.GetElapsedTime(StartTime), MeshStructureStore.Num());
	}
	else
	{
		PRTLog.Message(TEXT(">>> Generate Skip Count: "), ++GenerateSkipCount);
	}
	
	bAttributesUpdated = false;
	State = EPRTTaskState::GENTOMESH;
}

/**
 * \brief Copies Existing mesh data if it exists skipping Generate, else Generate
 * \param MeshStruct
 */
void APRTActor::GetModelData(TArray<FPRTMeshStruct>& MeshStruct)
{
	static double LastTimeStamp;
	
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("GetModelData called\n"));

	if (RPKFile == "(none)")
	{
		PRTLog.Message(FString(L">> APRTActor::GetModelData - RPKFile Undefined."), ELogVerbosity::Warning);
		State = EPRTTaskState::IDLE;
		bAttributesUpdated = false;
		return;
	}

	
	if(MeshStructureStore.Num() == 0)
	{
		bAttributesUpdated = true;
		GenerateModelData(true);
		return;
	}

	//const double Duration = PRTUtil.GetNowTime() - LastTimeStamp;
	//if (Duration < MinimumTimeBetweenRegens)
	//{
	//	// Interval short, so wait for more State Machine loops or Ticks or GetMesh calls
	//	PRTLog.Message(TEXT("Cache Interval short. Duration: "), Duration, ELogVerbosity::Warning);
	//	return;
	//}

	LastTimeStamp = PRTUtil.GetNowTime();
/*
	// We have an RPK file but the plugin isn't loaded
	if (PRT.IsLoaded() == false && RPKFile.Len() > 0)
	{
		InitializeRPKData(false);
		bAttributesUpdated = true;
		GenerateModelData();
		return;
	}
	else
	{
		CopyViewAttributesIntoDataStore();
	}

	CopyViewAttributesToAttributes();
*/

	CopyMeshStructures(MeshStructureStore, MeshStruct);

	State = EPRTTaskState::GENTOMESH;
}



#pragma endregion

#pragma region MeshCreation

/**
 * \brief Create a Mesh object from the MeshStruct data
 * \param bUseStaticMesh 
 */
void APRTActor::CreateMesh()
{
	// TODO:
	State = EPRTTaskState::MESHING;
	
	if(bUseStaticMesh)
	{
		// Static Mesh in UE 4.24 
	}
	else
	{
		for (auto& CurrentMeshStruct : MeshStructureStore)
		{
			// CurrentMeshStruct.Texture


		}
	}

	SetCollisionBox();
	
	State = EPRTTaskState::IDLE;
}



/**
 * \brief Configure the size, position, and rotation of the Collision Box component.
 */
void APRTActor::SetCollisionBox() const
{
	FVector NewLocation(0.0, 0.0, 0.0);
	FVector BoundingBox(2.0, 2.0, 2.0);
	PRTCollisionBox->SetBoxExtent(BoundingBox);
	PRTCollisionBox->SetRelativeLocation(NewLocation);
	FVector Origin;
	GetActorBounds(false, Origin, BoundingBox);

	const FVector ScaleVector(CollisionX_Scale, CollisionY_Scale, CollisionZ_Scale);
	const FVector TempExtents = BoundingBox * CollisionScale / 100.0;
	const FVector BoxScaled = TempExtents * ScaleVector;
	
	PRTCollisionBox->SetBoxExtent(BoxScaled);
	NewLocation.Set(0.0, 0.0, BoxScaled.Z);
	const FRotator NewRotation(0, CollisionRotation, 0.0);
	
	PRTCollisionBox->SetRelativeLocationAndRotation(NewLocation, NewRotation);
}

#pragma endregion


#pragma region Mesh Data Processing

/**
 * \brief Deep copy of the MeshStruct nested arrays from one to another.
 * Used for setting and retrieving the generated MeshStruct data and the cache.
 * \param SourceMeshStruct 
 * \param DestinationMeshStruct 
 */
void APRTActor::CopyMeshStructures(TArray<FPRTMeshStruct>& SourceMeshStruct, 
	TArray<FPRTMeshStruct>& DestinationMeshStruct)
{
	if (SourceMeshStruct.Num() == 0) return;
	if (DestinationMeshStruct.Num() != 0) DestinationMeshStruct.Empty();
	
	for (auto& CurrentMeshStruct : SourceMeshStruct)
	{
		FPRTMeshStruct TempMeshStruct;
		for(auto& CurrentIndices : CurrentMeshStruct.Indices)
		{
			int32 TempIndices = CurrentIndices;
			TempMeshStruct.Indices.Add(TempIndices);
		}

		for(auto& CurrentVertex : CurrentMeshStruct.Vertices)
		{
			FVector TempVertex;
			TempVertex.X = CurrentVertex.X;
			TempVertex.Y = CurrentVertex.Y;
			TempVertex.Z = CurrentVertex.Z;
			TempMeshStruct.Vertices.Add(TempVertex);			
		}

		for(auto& CurrentNormal : CurrentMeshStruct.Normals)
		{
			FVector TempNormal;
			TempNormal.X = CurrentNormal.X;
			TempNormal.Y = CurrentNormal.Y;
			TempNormal.Z = CurrentNormal.Z;
			TempMeshStruct.Normals.Add(TempNormal);
		}

		for(auto& CurrentUVs : CurrentMeshStruct.UVs)
		{
			FVector2D TempUV;
			TempUV.X = CurrentUVs.X;
			TempUV.Y = CurrentUVs.Y;
			TempMeshStruct.UVs.Add(TempUV);
		}
	
		for(auto& CurrentColor : CurrentMeshStruct.Colors)
		{
			FLinearColor TempColor;
			TempColor.R = CurrentColor.R;
			TempColor.G = CurrentColor.G;
			TempColor.B = CurrentColor.B;
			TempColor.A = CurrentColor.A;
			TempMeshStruct.Colors.Add(TempColor);
		}

		for(auto& CurrentTexture : CurrentMeshStruct.Texture)
		{
			uint8 TempTexture;
			TempTexture = CurrentTexture;
			TempMeshStruct.Texture.Add(TempTexture);
		}

		DestinationMeshStruct.Add(TempMeshStruct);
	}
}

void APRTActor::ProcessPRTVertexDataIntoMeshStruct(TArray<FPRTMeshStruct>& MeshStruct)
{
	double const StartTime = PRTUtil.GetNowTime();

	if (MeshStruct.Num() != 0)
	{
		MeshStruct.Empty();
	}

	PRTLog.Message(TEXT("> Processing Model Data..."));

	for (auto& CurrentVertexData : PRT.VertexData)
	{
		for (auto& CurrentMaterial : PRT.Materials)
		{
			FPRTMeshStruct MaterialMesh;

			FLinearColor TempColor;
			TempColor.R = CurrentMaterial.Value.Kd[0];
			TempColor.G = CurrentMaterial.Value.Kd[1];
			TempColor.B = CurrentMaterial.Value.Kd[2];
			TempColor.A = 1.0;

			TArray<float> Vertices = CurrentVertexData.Value.MaterialVertices[CurrentMaterial.Key];
			SetMaterialMeshVertexColors(&MaterialMesh, Vertices, TempColor);
			Vertices.Empty();

			TArray<float> Normals = CurrentVertexData.Value.MaterialNormals[CurrentMaterial.Key];
			SetMaterialMeshNormals(&MaterialMesh, Normals);
			Normals.Empty();

			TArray<float>& UVs = CurrentVertexData.Value.MaterialUVs[CurrentMaterial.Key];
			SetMaterialMeshUVs(&MaterialMesh, UVs);
			UVs.Empty();

			TArray<uint32_t>& Indices = CurrentVertexData.Value.MaterialIndices[CurrentMaterial.Key];
			SetMaterialMeshIndices(&MaterialMesh, Indices);
			Indices.Empty();

			if (PRT.JpegFiles.Contains(CurrentMaterial.Value.FileName))
			{
				const TArray<uint8> Data(PRT.JpegFiles[CurrentMaterial.Value.FileName],
				                   PRT.JpegSizes[CurrentMaterial.Value.FileName]);
				MaterialMesh.Texture = Data;
			}

			MeshStruct.Add(MaterialMesh);
		}
	}

	PRTLog.Message(TEXT(" > Mesh Processing Time: "), PRTUtil.GetElapsedTime(StartTime));
}

void APRTActor::SetMaterialMeshVertexColors(FPRTMeshStruct* MaterialMesh, TArray<float> Vertices,
                                            FLinearColor TempColor)
{
	for (int32 j = 0; j < Vertices.Num(); j += 3)
	{
		FVector Vector;
		Vector.X = Vertices[j];
		Vector.Z = Vertices[j + 1];
		Vector.Y = Vertices[j + 2];
		MaterialMesh->Vertices.Add(Vector);
		//add a color for each vertex.
		MaterialMesh->Colors.Add(TempColor);
	}
}

void APRTActor::SetMaterialMeshNormals(FPRTMeshStruct* MaterialMesh, TArray<float> Normals)
{
	for (int32 j = 0; j < Normals.Num(); j += 3)
	{
		FVector TempNormals;
		TempNormals.X = Normals[j];
		TempNormals.Z = Normals[j + 1];
		TempNormals.Y = Normals[j + 2];
		MaterialMesh->Normals.Add(TempNormals);
	}
}

void APRTActor::SetMaterialMeshUVs(FPRTMeshStruct* MaterialMesh, TArray<float> UVs)
{
	for (int32 j = 0; j < UVs.Num(); j += 2)
	{
		FVector2D TempUV;
		TempUV.X = UVs[j];
		TempUV.Y = 1.0 - UVs[j + 1];
		MaterialMesh->UVs.Add(TempUV);
	}
}

void APRTActor::SetMaterialMeshIndices(FPRTMeshStruct* MaterialMesh, TArray<uint32_t>& Indices)
{
	for (int32 j = 0; j < Indices.Num(); j++)
	{
		MaterialMesh->Indices.Add(Indices[j]);
	}
}


void CreateStaticMesh()
{
}

#pragma endregion


#pragma region Attributes

/**
 * \brief
 */
void APRTActor::ClearViewAttributesDataStoreCache()
{
	for (auto i = 0; i < ViewAttributesDataStore.Num(); i++)
	{
		//empty the array
		TArray<FCEGroup>& LocalViewAttributes = ViewAttributesDataStore[i].ViewAttributes;
		for (auto Index = 0; Index < LocalViewAttributes.Num(); Index++)
		{
			for (auto j = 0; j < LocalViewAttributes[Index].Attributes.Num(); j++)
			{
#if WITH_EDITOR
				LocalViewAttributes[Index].Attributes[j].SlateWidget.Destroy();
#endif
				LocalViewAttributes[Index].Attributes[j].Arguments.Empty();
			}
			LocalViewAttributes[Index].Attributes.Empty();
		}
		LocalViewAttributes.Empty();
	}
	ViewAttributesDataStore.Empty();

	//PRTLog.Message(TEXT("Cleared attributes cache map."));
}

void APRTActor::CopyViewAttributesToAttributes()
{
	//if (Attributes.Num() == 0)
	{
		for (auto i = 0; i < ViewAttributes.Num(); i++)
		{
			for (auto j = 0; j < ViewAttributes[i].Attributes.Num(); j++)
			{
				FString Name = ViewAttributes[i].Attributes[j].Name;
				Attributes[Name].KeyName = Name;
				Attributes[Name].bValue = ViewAttributes[i].Attributes[j].bValue;
				Attributes[Name].fValue = ViewAttributes[i].Attributes[j].fValue;
				Attributes[Name].sValue = ViewAttributes[i].Attributes[j].sValue;
			}
		}
	}
}

void APRTActor::EraseAttributes()
{
	//The PRTPlugin handles erasing attributes themselves. But let's do it here anyways.
	for (auto& i : Attributes)
	{
		i.Value.Arguments.Empty();
	}

	Attributes.Empty();
	//empty the array

	for (auto i = 0; i < ViewAttributes.Num(); i++)
	{
		for (auto j = 0; j < ViewAttributes[i].Attributes.Num(); j++)
		{
#if WITH_EDITOR
			ViewAttributes[i].Attributes[j].SlateWidget.Destroy();
#endif
			ViewAttributes[i].Attributes[j].Arguments.Empty();
		}
		ViewAttributes[i].Attributes.Empty();
	}
	ViewAttributes.Empty();
}

void APRTActor::InitializeViewAttributes()
{
	BuildNewViewAttributeArray();
	SortViewAttributesArray();
}

void APRTActor::BuildNewViewAttributeArray()
{
	FString Group;
	int32 GroupOrder;
	int32 AttributeOrder = INT_MAX;

	for (auto& CurrentAttribute : Attributes)
	{
		Group = "";
		FCEAttribute Attribute;
		AttributeOrder = INT_MAX;

		// Loop through all of the Attributes 
		// Build an Attribute structure from the current attribute
		// Save it to the ViewAttributes global struct.
		if (CurrentAttribute.Value.KeyName.Len() > 0)
		{
			// Reset Defaults
			Attribute.bHidden = false;
			Attribute.showInVR = false;
			Attribute.Step = .1;

			CreateArguments(&Attribute, CurrentAttribute, &Group, &GroupOrder, &AttributeOrder);
			SetAttributeType(&Attribute, CurrentAttribute.Value.Type);
			SetAlternateWidgetType(&Attribute);
			CreateDisplayName(&Attribute, CurrentAttribute.Value.KeyName);
			AddAttributeToViewAttributes(Attribute, Group, GroupOrder);
		}
	}
}

/**
 * \brief Main Loop for parsing attributes and creating arguments
 */
void APRTActor::CreateArguments(FCEAttribute* Attribute, TPair<FString, FPRTAttribute> CurrentAttribute, FString* Group,
	int32* GroupOrder, int32* AttributeOrder)
{
	for (auto j = 0; j < CurrentAttribute.Value.Arguments.Num(); j++)
	{
		FCEArgument Argument;
		Argument.Name = CurrentAttribute.Value.Arguments[j].KeyName;

		SetArgumentType(&Argument, CurrentAttribute.Value.Arguments[j].Type);
		SetArgumentValues(&Argument, CurrentAttribute.Value.Arguments[j].bValue,
			CurrentAttribute.Value.Arguments[j].fValue,
			CurrentAttribute.Value.Arguments[j].sValue);
		SetAttributeParametersAndWidgets(&Argument, Attribute, Group, GroupOrder, AttributeOrder);

		Attribute->Arguments.Add(Argument);
	}

	Attribute->bValue = CurrentAttribute.Value.bValue;
	Attribute->fValue = CurrentAttribute.Value.fValue;
	Attribute->sValue = CurrentAttribute.Value.sValue;
	Attribute->Order = *AttributeOrder;
}

/**
 * \brief Set Argument Type (Bool, Float, String)
 */
void APRTActor::SetArgumentType(FCEArgument* Argument, int32 Type)
{
	switch (Type)
	{
	case prt::AAT_BOOL:
		Argument->Type = 0;
		break;
	case prt::AAT_FLOAT:
		Argument->Type = 1;
		break;
	case prt::AAT_STR:
		Argument->Type = 2;
		break;
	default:;
	}
}

/**
 * \brief Argument Bool, String, and Float values set from input vars
 */
void APRTActor::SetArgumentValues(FCEArgument* Argument, bool bValue, float fValue, FString sValue)
{
	Argument->bValue = bValue;
	Argument->fValue = fValue;
	Argument->sValue = sValue;
}

/**
 * \brief Handle the Argument @ Name Types and Attribute add the new Argument to Attributes
 */
void APRTActor::SetAttributeParametersAndWidgets(FCEArgument* Argument, FCEAttribute* Attribute, FString* Group,
	int32* GroupOrder, int32* AttributeOrder)
{
	if (Argument->Name.Compare("@Color") == 0)
	{
		Attribute->Widget = ERPKWidgetTypes::COLOR;
	}

	if (Argument->Name.Compare("@Hidden") == 0)
	{
		Attribute->bHidden = true;
	}

	// TODO: unfortunately it does not appear the @Hidden argument is coming through.				
	if (Argument->Name.Compare("@Percent") == 0)
	{
		Attribute->bIsPercentage = true;
	}

	if (Argument->Name.Compare("@Group") == 0)
	{
		switch (Argument->Type)
		{
		case 1:
			*GroupOrder = Argument->fValue;
			break;
		case 2:
			*Group = Argument->sValue;
			break;
		default:;
		}
	}

	// Range is a Slider for Floats or a Combobox for Strings
	if (Argument->Name.Compare("@Range") == 0)
	{
		// Range Type 1 Float Value
		if (Argument->Type == 1)
		{
			// Less than two options
			if (Attribute->Range.Num() < 2)
			{
				Attribute->Range.Add(Argument->fValue);
				if (Attribute->Range.Num() == 2)
				{
					Attribute->SliderStep = .1 / (Attribute->Range[1] - Attribute->Range[0]);
				}
			}
			else
			{
				// More than two @Range options: Step, Min, Max
				Attribute->Step = Argument->fValue;
				Attribute->SliderStep = Argument->fValue / (Attribute->Range[1] - Attribute->Range[0]);
			}

			// Attribute @Range options > 1, enable Slider
			if (Attribute->Range.Num() > 1)
			{
				Attribute->Widget = ERPKWidgetTypes::SLIDER;
			}
		}

		// Range Type 2 is a Selection Combo Box
		if (Argument->Type == 2)
		{
			Attribute->SelectValues.Add(Argument->sValue);
			Attribute->Widget = ERPKWidgetTypes::COMBO;
		}
	}

	// Enum elements can be a Float or String value. Always a Combo Box
	if (Argument->Name.Compare("@Enum") == 0)
	{
		// Float Value
		if (Argument->Type == 1)
		{
			Attribute->SelectValues.Add(FString::SanitizeFloat(Argument->fValue));
		}
		// String Value
		if (Argument->Type == 2)
		{
			Attribute->SelectValues.Add(Argument->sValue);
		}
		Attribute->Widget = ERPKWidgetTypes::COMBO;
	}

	if (Argument->Name.Compare("@Order") == 0)
	{
		*AttributeOrder = Argument->fValue;
	}

	if (Argument->Name.Compare("@File") == 0)
	{
		Attribute->Widget = ERPKWidgetTypes::FILE;
	}

	if (Argument->Name.Compare("@Directory") == 0)
	{
		Attribute->Widget = ERPKWidgetTypes::DIRECTORY;
	}
}


/**
 * \brief Set Attribute Type (Bool, Float, or String) based on Type value (0, 1, 2)
 */
void APRTActor::SetAttributeType(FCEAttribute* Attribute, int32 Type)
{
	switch (Type)
	{
	case prt::AAT_BOOL:
		Attribute->Type = 0;
		break;
	case prt::AAT_FLOAT:
		Attribute->Type = 1;
		break;
	case prt::AAT_STR:
		Attribute->Type = 2;
		// Color are type string, a specific length, and start with a #
		if (Attribute->sValue.Len() == 7 && Attribute->sValue[0] == '#')
		{
			Attribute->Widget = ERPKWidgetTypes::COLOR;
		}
		break;
	default: break;
	}
}

void APRTActor::SortViewAttributesArray()
{
	for (auto i = 0; i < ViewAttributes.Num(); i++)
	{
		for (auto j = i + 1; j < ViewAttributes.Num(); j++)
		{
			if (ViewAttributes[i].Order > ViewAttributes[j].Order)
			{
				const FCEGroup CeGroup = ViewAttributes[i];
				ViewAttributes[i] = ViewAttributes[j];
				ViewAttributes[j] = CeGroup;
			}
		}

		for (auto k = 0; k < ViewAttributes[i].Attributes.Num(); k++)
		{
			for (int32 l = k + 1; l < ViewAttributes[i].Attributes.Num(); l++)
			{
				if (ViewAttributes[i].Attributes[k].Order > ViewAttributes[i].Attributes[l].Order)
				{
					const FCEAttribute CeAttribute = ViewAttributes[i].Attributes[k];
					ViewAttributes[i].Attributes[k] = ViewAttributes[i].Attributes[l];
					ViewAttributes[i].Attributes[l] = CeAttribute;
				}
			}
		}
	}
}

void APRTActor::CreateDisplayName(FCEAttribute* Attribute, FString KeyName)
{
	Attribute->Name = FString(KeyName);
	const int32 Start = Attribute->Name.Find("$") + 1;
	Attribute->DisplayName = Attribute->Name.Mid(Start, Attribute->Name.Len() - Start);
	for (auto j = 0; j < Attribute->DisplayName.Len(); j++)
	{
		if (Attribute->DisplayName[j] == '_')
		{
			Attribute->DisplayName[j] = ' ';
		}
	}
}

/**
 * \brief Modified the Widget Type if needed
 */
void APRTActor::SetAlternateWidgetType(FCEAttribute* Attribute)
{
	// Number input, not text
	if (Attribute->Type == 1 && Attribute->Widget == ERPKWidgetTypes::GENERAL_TEXT)
	{
		Attribute->Widget = ERPKWidgetTypes::NUMBER_TEXT;
	}

	// Force Boolean to a Checkbox
	if (Attribute->Type == 0)
	{
		Attribute->Widget = ERPKWidgetTypes::CHECKBOX;
	}

	//Change the string to a color, if color was detected:
	if (Attribute->Widget == ERPKWidgetTypes::COLOR)
	{
		Attribute->Color = FLinearColor(FColor::FromHex(Attribute->sValue));
	}
}

void APRTActor::AddAttributeToViewAttributes(const FCEAttribute Attribute, const FString Group, const int32 GroupOrder)
{
	bool bInjected = false;

	// Loop through ViewAttributes and see if Name is same as Group
	for (auto j = 0; j < ViewAttributes.Num(); j++)
	{
		//See if the Group name exists
		if (ViewAttributes[j].Name.Compare(Group) == 0)
		{
			// ViewAttributes $Name == $Group, so add the Attribute to the array of attribs
			ViewAttributes[j].Attributes.Add(Attribute);
			bInjected = true;
		}
	}

	// Names didn't match, so make a new group
	if (!bInjected)
	{
		FCEGroup CeGroup;
		CeGroup.Name = Group;
		CeGroup.Order = GroupOrder;
		CeGroup.Attributes.Add(Attribute);
		ViewAttributes.Add(CeGroup);
	}
}
#pragma endregion


#pragma region Synchronization

/******************************************************************************
*		Attribute Synchronization.
*		Used to keep all the data synced for VR, Slate and the plugin.
*		call these methods when you want to change a Value.
******************************************************************************/
void APRTActor::SyncAttributeColor(const int32 GroupIndex, const int32 AttributeIndex, const FLinearColor Value)
{
	if (ViewAttributes.Num() > GroupIndex)
	{
		bAttributesUpdated = true;
		if (ViewAttributes[GroupIndex].Attributes.Num() > AttributeIndex)
		{
			FCEAttribute& LocalAttribute = ViewAttributes[GroupIndex].Attributes[AttributeIndex];
			LocalAttribute.Color = Value;
			LocalAttribute.sValue = L"#" + Value.ToRGBE().ToHex().Mid(0, 6);
#if WITH_EDITOR
			LocalAttribute.SlateWidget.Update();
#endif
			CopyViewAttributesIntoDataStore();
		}
	}
}


void APRTActor::SyncAttributeString(const int32 Group_Index, const int32 Attribute_Index, const FString Value)
{
	
	if (ViewAttributes.Num() > Group_Index)
	{
		bAttributesUpdated = true;
		if (ViewAttributes[Group_Index].Attributes.Num() > Attribute_Index)
		{
			FCEAttribute& LocalAttribute = ViewAttributes[Group_Index].Attributes[Attribute_Index];
			LocalAttribute.sValue = Value;
			const FString Name = *LocalAttribute.Name;
			Attributes[Name].bValue = LocalAttribute.bValue;
			Attributes[Name].fValue = LocalAttribute.fValue;
			Attributes[Name].sValue = *LocalAttribute.sValue;
#if WITH_EDITOR
			LocalAttribute.SlateWidget.Update();
#endif
			CopyViewAttributesIntoDataStore();
		}
	}
}

void APRTActor::SyncAttributeFloat(const int32 Group_Index, const int32 Attribute_Index, const float Value)
{
	if (ViewAttributes.Num() > Group_Index)
	{
		bAttributesUpdated = true;
		if (ViewAttributes[Group_Index].Attributes.Num() > Attribute_Index)
		{
			FCEAttribute& LocalAttribute = ViewAttributes[Group_Index].Attributes[Attribute_Index];
			LocalAttribute.fValue = Value;
			const FString Name = *LocalAttribute.Name;
			Attributes[Name].bValue = LocalAttribute.bValue;
			Attributes[Name].fValue = LocalAttribute.fValue;
			Attributes[Name].sValue = *LocalAttribute.sValue;
#if WITH_EDITOR
			LocalAttribute.SlateWidget.Update();
#endif
			CopyViewAttributesIntoDataStore();
		}
	}
}

void APRTActor::SyncAttributeBool(const int32 Group_Index, const int32 Attribute_Index, const bool bValue)
{
	if (ViewAttributes.Num() > Group_Index)
	{
		bAttributesUpdated = true;
		if (ViewAttributes[Group_Index].Attributes.Num() > Attribute_Index)
		{
			FCEAttribute& LocalAttribute = ViewAttributes[Group_Index].Attributes[Attribute_Index];
			LocalAttribute.bValue = bValue;
			const FString Name = *LocalAttribute.Name;
			Attributes[Name].bValue = LocalAttribute.bValue;
			Attributes[Name].fValue = LocalAttribute.fValue;
			Attributes[Name].sValue = *LocalAttribute.sValue;
#if WITH_EDITOR
			LocalAttribute.SlateWidget.Update();
#endif
			CopyViewAttributesIntoDataStore();
		}
	}
}

void APRTActor::RecallViewAttributes()
{
	bAttributesUpdated = true;
	bool bFound = false;
	for (auto i = 0; i < ViewAttributesDataStore.Num(); i++)
	{
		if (ViewAttributesDataStore[i].RPKFile.Compare(RPKFile) == 0)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue, FString(L"Restoring View Attributes."));
			//PRTLog.Message(FString(L"Restoring View Attributes."));
			ViewAttributes = ViewAttributesDataStore[i].ViewAttributes;
			bFound = true;
		}
	}
	if (!bFound)
	{
		// It isn't an error or an issue, so no message
		// PRTLog.Message(FString(L"No View Attributes Found to Restore."));
	}
}


/**
 * \brief The current ViewAttributes are copied into ViewAttributesDataStore
 */
void APRTActor::CopyViewAttributesIntoDataStore()
{
	if (PreviousRPKFile.Len() > 0 && PreviousRPKFile.Compare(L"(none)") != 0)
	{
		if (ViewAttributes.Num() > 0)
		{
			FCERPKViewAttributes RPKViewAttributes;
			RPKViewAttributes.RPKFile = PreviousRPKFile;
			RPKViewAttributes.ViewAttributes = ViewAttributes;
			for (int32 i = 0; i < RPKViewAttributes.ViewAttributes.Num(); i++)
			{
				for (int32 j = 0; j < RPKViewAttributes.ViewAttributes[i].Attributes.Num(); j++)
				{
#if WITH_EDITOR
					RPKViewAttributes.ViewAttributes[i].Attributes[j].SlateWidget.Destroy();
#endif
				}
			}

			bool bFound = false;
			for (auto i = 0; i < ViewAttributesDataStore.Num(); i++)
			{
				if (ViewAttributesDataStore[i].RPKFile.Compare(PreviousRPKFile) == 0)
				{
					ViewAttributesDataStore[i].ViewAttributes = ViewAttributes;
					bFound = true;
				}
			}
			if (!bFound)
			{
				ViewAttributesDataStore.Add(RPKViewAttributes);
			}
		}
	}
	PreviousRPKFile = RPKFile;
}


bool APRTActor::InPIE()
{
	if (!PRTCollisionBox) return false;

	UWorld* const World = GEngine->GetWorldFromContextObject(PRTCollisionBox, EGetWorldErrorMode::ReturnNull);
	if (!World) return false;

	return (World->WorldType == EWorldType::PIE);
}

#pragma endregion
