// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <CoreMinimal.h>
#include <DesktopPlatform/Public/IDesktopPlatform.h>
//#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include <GameFramework/Actor.h>
#include <PRTModule.h>
//#include "InputCoreTypes.h"
//#include <Slate.h>

#include "ProceduralMeshComponent.h"

#if WITH_EDITOR
#include <RPKWidget.h>
//#include "Editor.h"
//#include "PropertyEditorModule.h"
#endif

#include <PrtDetail.h>
#include "PRTUtilities.h"

//#include "SEditableTextBox.h"
//#include "SCompoundWidget.h"
//#include "Widgets/Colors/SColorPicker.h"

//#include <EngineGlobals.h>
//#include <Runtime/Engine/Classes/Engine/Engine.h>
//#include <Runtime/Engine/Classes/Engine/StaticMesh.h>

#include "PRTActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGenerateDelegate);

#pragma region Enums and Structs

UENUM(BlueprintType)
enum class ERPKWidgetTypes : uint8
{
	GENERAL_TEXT UMETA(DisplayName="General Text"),
	SLIDER UMETA(DisplayName="Number Slider"),
	COLOR UMETA(DisplayName="Color Picker"),
	COMBO UMETA(DisplayName="Combo Box"),
	FILE UMETA(DisplayName="File Picker"),
	DIRECTORY UMETA(DisplayName="Directory Picker"),
	CHECKBOX UMETA(DisplayName="Checkbox"),
	NUMBER_TEXT UMETA(DisplayName="Number Input")
};

UENUM(BlueprintType)
enum class EPRTLogVerbosity : uint8
{
	LOG_ALL UMETA(DisplayName = "All"),
	LOG_DISPLAY UMETA(DisplayName = "Display or higher"),
	LOG_WARNING UMETA(DisplayName = "Warning or higher"),
	LOG_ERROR UMETA(DisplayName = "Only Error messages"),
};


UENUM(BlueprintType)
enum class EPRTTaskState : uint8
{
	IDLE UMETA(DisplayName = "Idle"),
	IDLETOGEN UMETA(DisplayName = "Idle to Generating"),
	GENERATING UMETA(DisplayName = "Generating"),
	GENTOMESH UMETA(DisplayName = "Generating to Meshing"),
	MESHING UMETA(DisplayName = "Meshing"),
	MESHINGTOCOMPL UMETA(DisplayName = "Meshing to Complete"),
	COMPLETE UMETA(DisplayName = "Complete")
};

UENUM(BlueprintType)
enum class ERPKLogType : uint8
{
	DISPLAY UMETA(DisplayName = "Display"),
	WARNING UMETA(DisplayName = "Warning"),
	ERRORMSG UMETA(DisplayName = "Error")
};


USTRUCT(BlueprintType)
struct FRPKFile
{
	GENERATED_USTRUCT_BODY();
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="RPK File")
	FString Name;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK File")
	FString Path;
};

USTRUCT(BlueprintType)
struct FOBJFile
{
	GENERATED_USTRUCT_BODY();
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "OBJ File")
	FString Name;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "OBJ File")
	FString Path;
};

USTRUCT(BlueprintType)
struct FPRTArgument
{
	GENERATED_USTRUCT_BODY();
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT Argument")
	FString Keyname;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT Argument")
	int32 Type;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT Argument")
	bool bValue;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT Argument")
	FString sValue;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT Argument")
	float fValue;
};

USTRUCT(BlueprintType)
struct FPRTMeshStruct
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT Mesh Struct")
	TArray<int32> Indices;

	//the following all need to be the exact same size.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT Mesh Struct")
	TArray<FVector> Vertices;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT Mesh Struct")
	TArray<FVector> Normals;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT Mesh Struct")
	TArray<FVector2D> UVs;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT Mesh Struct")
	TArray<FLinearColor> Colors;

	//this is the texture for the mesh
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT Mesh Struct")
	TArray<uint8> Texture;
};

USTRUCT(BlueprintType)
struct FCEArgument
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "CE Argument")
	FString Name;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CE Argument")
	int32 Type;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CE Argument")
	float fValue;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CE Argument")
	FString sValue;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CE Argument")
	bool bValue;
};

USTRUCT(BlueprintType)
struct FCEAttribute
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="CE Attribute")
	FString DisplayName;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="CE Attribute")
	FString Name;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="CE Attribute")
	int32 Type;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="CE Attribute")
	float fValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="CE Attribute")
	FString sValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="CE Attribute")
	FLinearColor Color;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="CE Attribute")
	bool bValue;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="CE Attribute")
	TArray<FCEArgument> Arguments;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="CE Attribute")
	bool showInVR = false;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="CE Attribute")
	TArray<float> Range;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="CE Attribute")
	float Step;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="CE Attribute")
	float SliderStep;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="CE Attribute")
	TArray<FString> SelectValues;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="CE Attribute")
	bool bIsPercentage = false;
	bool bHidden = false;
	int32 Order = 0;

	//the type of widget to use:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="CE Attribute")
	ERPKWidgetTypes Widget = ERPKWidgetTypes::GENERAL_TEXT;
#if WITH_EDITOR
	//now we pack a class with all of the slate data and callbacks.
	FRPKWidget SlateWidget;
#endif
};

USTRUCT(BlueprintType)
struct FCEGroup
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="CE Group")
	FString Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="CE Group")
	TArray<FCEAttribute> Attributes;

	int32 Order;
};

USTRUCT(BlueprintType)
struct FCERPKViewAttributes
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="CE RPK View Attributes")
	FString RPKFile;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="CE RPK View Attributes")
	TArray<FCEGroup> ViewAttributes;
};


USTRUCT(BlueprintType)
struct FPRTLogMessage
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CE RPK Log")
	uint8 Type;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CE RPK Log")
	FString Message;
};

USTRUCT(BlueprintType)
struct FPRTReportMessage
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CE RPK Log")
	FString Key;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CE RPK Log")
	FString Value;
};

#pragma endregion


UCLASS(Blueprintable)
class PRT_API APRTActor : public AActor
{
	GENERATED_BODY()

public:
	APRTActor();

	// Sets default values for this actor's properties
	TMap<FString, FPRTAttribute> Attributes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Reporting")
	TArray<FPRTReportMessage> Reports;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Reporting")
	TArray<FPRTLogMessage> LogMessages;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Utility")
	TArray<FRPKFile> RPKFiles;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Utility")
	TArray<FOBJFile> OBJFiles;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Utility")
	bool bUseHardcodedValues = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Threading")
	EPRTTaskState State;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "RPK Threading")
	bool bUseThreading = true;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "RPK Threading")
	bool bGenerate = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Threading")
	int32 GenerateCount;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Threading")
		int32 GenerateSkipCount;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Threading")
	float CurrentGenerationTime;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Threading")
	float LastGenerationElapsedTime;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Threading")
	float MeshingTime;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "RPK Threading")
	float MinimumTimeBetweenRegens = 0.10f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Threading")
	float IdleTime;

	//		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Utility")
	//		  FProceduralMeshComponent	 RPKProceduralMesh;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "RPK Utility")
		bool bAutoGenerate = false;
	
	// Read/Write so Actor or Server can set Dirty status, too.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Utility")
	bool bAttributesUpdated = true;	// Allow regen on first run

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Utility")
	FString RPKPath = L"";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Utility")
	FString RPKFile = L"(none)";
	FString PreviousRPKFile = L""; //usable only by CopyViewAttributesIntoDataStore;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Utility")
	FString OBJPath = L"";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Utility")
	FString OBJFile = L"square";

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Utility")
	TArray<FString> Rules;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "RPK Utility")
	bool bHasEditor = false;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "RPK Components")
	UStaticMeshComponent* PRTStaticMesh;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "RPK Components")
	UProceduralMeshComponent* PRTProceduralMesh;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "RPK Components")
	UBoxComponent* PRTCollisionBox;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "RPK Utility")
		EPRTLogVerbosity LogVerbosity = EPRTLogVerbosity::LOG_WARNING;

	// RPK File Attribute arrays
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Utility")
	TArray<FCEGroup> ViewAttributes;

	//the permanent Cache for Attributes.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Utility")
	TArray<FCERPKViewAttributes> ViewAttributesDataStore;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Collision Box")
		float CollisionX_Scale = 0.5;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Collision Box")
		float CollisionY_Scale = 0.5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Collision Box")
		float CollisionZ_Scale = 1.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Collision Box")
		float CollisionScale = 1.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Collision Box")
		float CollisionRotation = 0.0;

	
	UFUNCTION(BlueprintCallable, Category = PRT, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
	void SetCollisionBox() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = PRT, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
	bool InPIE();

	UFUNCTION(BlueprintCallable, Category = PRT, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
	void InitializeStateMachine();

	/*Copy the ViewAttributes from the current RPK and store them in ViewAttributesDataStore*/
	UFUNCTION(BlueprintCallable, Category = "RPK Utility", meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
	void CopyViewAttributesIntoDataStore();

	/*Retrieves the ViewAttributes from ViewAttributesDataStore for the current RPK */
	UFUNCTION(BlueprintCallable, Category = "RPK Utility", meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
	void RecallViewAttributes();

	/*Sets the current RPK Path, initial shape and refreshes the building*/
	UFUNCTION(BlueprintCallable, Category = PRT, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
	void InitializeRPKData(bool bCompleteReset);

	/*Sets the current RPK Path, initial shape and refreshes the building*/
	UFUNCTION(BlueprintCallable, Category = PRT, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
		void SetRPKState(EPRTTaskState NewStatus);

	/*Sets the current RPK Path, initial shape and refreshes the building*/
	UFUNCTION(BlueprintCallable, Category = PRT, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
		bool CompareRPKState(EPRTTaskState CompareStatus) const;
	
	/*Generate the building and create mesh*/
	//UFUNCTION(BlueprintCallable, Category = PRT)
	//void GenerateBuilding();

	/*attempt to load the RPK if we already have a file specified, this is mostly for use on opening a project,
	 *and restoring	the actor to the previous state.*/
	UFUNCTION(BlueprintCallable, Category = PRT, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
		void GetModelData(TArray<FPRTMeshStruct>& MeshStruct);

	UFUNCTION(BlueprintCallable, Category = PRT, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
		void CreateMesh();

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = PRT, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
		bool bUseStaticMesh = false;

	/* attempt to load the RPK if we already have a file specified, this is mostly for use on opening a project,
	 * and restoring	the actor to the previous state.
	 * Future: Create the Static Mesh using UE 4.24+ */
	UFUNCTION(BlueprintCallable, Category = PRT, meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
		void GenerateModelData(bool bForceRegen = false);
	
#pragma region Sync Data
	/*
	 *Store the specific string attribute from the current RPK into the Attributes Map before updating the building
	 *@param GroupIndex - index of ViewAttributes
	 *@param AttributeIndex - index of Attributes inside of ViewAttributes
	 *@param Value - The new string value to be used in generating the building
	 **/
	UFUNCTION(BlueprintCallable, Category = "RPK Utility")
	void SyncAttributeString(int32 GroupIndex, int32 AttributeIndex, FString Value);
	
	/*
	 *Store the specific float attribute from the current RPK into the Attributes Map before updating the building
	 *@param GroupIndex - index of ViewAttributes
	 *@param AttributeibuteIndex - index of Attributes inside of ViewAttributes
	 *@param Value - The new float value to be used in generating the building
	 **/
	UFUNCTION(BlueprintCallable, Category = "RPK Utility")
	void SyncAttributeFloat(int32 GroupIndex, int32 AttributeIndex, float Value);
	
	/*
	 *Store the specific bool attribute from the current RPK into the Attributes Map before updating the building
	 *@param GroupIndex - index of ViewAttributes
	 *@param AttributeibuteIndex - index of Attributes inside of ViewAttributes
	 *@param Value - The new bool value to be used in generating the building
	 **/
	UFUNCTION(BlueprintCallable, Category = "RPK Utility")
	void SyncAttributeBool(int32 GroupIndex, int32 AttributeIndex, bool bValue);
	
	/*
	 *Store the specific color attribute from the current RPK into the Attributes Map before updating the building
	 *@param GroupIndex - index of ViewAttributes
	 *@param AttributeibuteIndex - index of Attributes inside of ViewAttributes
	 *@param Value - The new color value to be used in generating the building
	 **/
	UFUNCTION(BlueprintCallable, Category = "RPK Utility")
	void SyncAttributeColor(int32 GroupIndex, int32 AttributeIndex, FLinearColor Value);
#pragma endregion
	
	/*
	 *Creates the procedural mesh
	 *@note Full Function implemented in Blueprint RPKDecoderActor
	 *@note this only works with the following 3 options "BlueprintImplementableEvent, BlueprintCallable, CallInEditor"
	 *@note Which unfortunately "CallInEditor" exposes the button to the editor, which
	 *@note sometime in the future, we need to mask from the user.
	*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, CallInEditor, Category = "RPK Utility", meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
	void Generate(bool bForceRegen = false);

	/*
	 *Creates the procedural mesh
	 *@note Full Function implemented in Blueprint RPKDecoderActor
	 *@note this only works with the following 3 options "BlueprintImplementableEvent, BlueprintCallable, CallInEditor"
	 *@note Which unfortunately "CallInEditor" exposes the button to the editor, which
	 *@note sometime in the future, we need to mask from the user.
	*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, CallInEditor, Category = "RPK Utility", meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
		void ProcessGeneratedData();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, CallInEditor, Category = "RPK Utility", meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
	void CreateStaticMesh();

	UFUNCTION(BlueprintCallable, BlueprintCallable, CallInEditor, Category = "RPK Utility", meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
	void ClearViewAttributesDataStoreCache();

	/*Searches for RPK and OBJ files in the current working directory and populates a list of RPKS for later use*/
	auto BuildFileLists(bool bRescan = false) -> void;

	void GetObjFileList(IFileManager& FileManager, FString ContentDir);
	void GetRPKFileList(IFileManager& FileManager, FString ContentDir);

	// Called every frame
	void Tick(float DeltaTime) override;
#if WITH_EDITOR
	FPrtDetail* PrtDetail = nullptr;
#endif


private:
	FPRTModule PRT;
	FPRTLog PRTLog;
	FPRTUtilities PRTUtil;
	bool bInitialized = false;
	double LastGenerationTimestamp = 0;
	TArray<FMeshDescription*> MeshDescriptions;

#pragma region Meshing
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RPK Utility")
	TArray<FPRTMeshStruct> MeshStructureStore;

	void GetComponents();

	void CreateMaterialInstance(TArray<uint8> Texture);
	
	static void CopyMeshStructures(TArray<FPRTMeshStruct>& SourceMeshStruct, TArray<FPRTMeshStruct>& DestinationMeshStruct);
	void ProcessPRTVertexDataIntoMeshStruct(TArray<FPRTMeshStruct>& MeshStruct);
	static void SetMaterialMeshVertexColors(FPRTMeshStruct* MaterialMesh, TArray<float> Vertices, FLinearColor TempColor);
	static void SetMaterialMeshNormals(FPRTMeshStruct* MaterialMesh, TArray<float> Normals);
	static void SetMaterialMeshUVs(FPRTMeshStruct* MaterialMesh, TArray<float> UVs);
	static void SetMaterialMeshIndices(FPRTMeshStruct* MaterialMesh, TArray<uint32_t>& Indices);
#pragma endregion

#pragma region Attributes
	
	void EraseAttributes();
	void InitializeViewAttributes();
	void BuildNewViewAttributeArray();
	void SortViewAttributesArray();
	void UseFirstRule();

	void CopyViewAttributesToAttributes();

	static void CreateArguments(FCEAttribute* Attribute, TPair<FString, FPRTAttribute> CurrentAttribute, FString* Group,
	                            int32* GroupOrder, int32* AttributeOrder);
	static void SetArgumentType(FCEArgument* Argument, int32 TheType);
	static void SetArgumentValues(FCEArgument* Argument, bool bValue, float fValue, FString sValue);
	static void SetAttributeParametersAndWidgets(FCEArgument* Argument, FCEAttribute* Attribute, FString* Group,
	                                             int32* GroupOrder, int32* AttributeOrder);
	static void SetAttributeType(FCEAttribute* Attribute, int32 Type);
	static void CreateDisplayName(FCEAttribute* Attribute, FString KeyName);
	static void SetAlternateWidgetType(FCEAttribute* Attribute);
	void AddAttributeToViewAttributes(FCEAttribute Attribute, FString Group, int32 GroupOrder);
#pragma endregion
	
protected:
	// Called when the game starts or when spawned
	void BeginPlay() override;
};
