// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <CoreMinimal.h>
#include <DesktopPlatform/Public/IDesktopPlatform.h>
#include <GameFramework/Actor.h>
#include "ProceduralMeshComponent.h"

#if WITH_EDITOR
#include <RPKWidget.h>
#endif

#include <VitruvioModule.h>
#include <PRTGenerator.h>
#include <PrtDetail.h>
#include <PRTUtilities.h>

#include "PRTActor.generated.h"

class FGenerator;


#pragma region Enums and Structs

UENUM(BlueprintType, Category = "PRT|Enum")
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

UENUM(BlueprintType, Category = "PRT|Enum")
enum class EPRTLogVerbosity : uint8
{
	LOG_ALL UMETA(DisplayName = "All"),
	LOG_DISPLAY UMETA(DisplayName = "Display or higher"),
	LOG_WARNING UMETA(DisplayName = "Warning or higher"),
	LOG_ERROR UMETA(DisplayName = "Only Error messages"),
};

UENUM(BlueprintType, Category = "PRT|Enum")
enum class ERPKLogType : uint8
{
	DISPLAY UMETA(DisplayName = "Display"),
	WARNING UMETA(DisplayName = "Warning"),
	ERRORMSG UMETA(DisplayName = "Error")
};


USTRUCT(BlueprintType, Category = "PRT|Struct")
struct FRPKFile
{
	GENERATED_USTRUCT_BODY();
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="PRT|RPK File")
	FString Name;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|RPK File")
	FString Path;
};

USTRUCT(BlueprintType, Category = "PRT|Struct")
struct FOBJFile
{
	GENERATED_USTRUCT_BODY();
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|OBJ File")
	FString Name;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|OBJ File")
	FString Path;
};

USTRUCT(BlueprintType, Category = "PRT|Struct")
struct FPRTArgument
{
	GENERATED_USTRUCT_BODY();
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|Argument")
	FString Keyname;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|Argument")
	int32 Type;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|Argument")
	bool bValue;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|Argument")
	FString sValue;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|Argument")
	float fValue;
};

USTRUCT(BlueprintType, Category = "PRT|Struct")
struct FPRTMeshStruct
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|Mesh Struct")
	TArray<int32> Indices;

	//the following all need to be the exact same size.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|Mesh Struct")
	TArray<FVector> Vertices;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|Mesh Struct")
	TArray<FVector> Normals;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|Mesh Struct")
	TArray<FVector2D> UVs;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|Mesh Struct")
	TArray<FLinearColor> Colors;

	//this is the texture for the mesh
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|Mesh Struct")
	TArray<uint8> Texture;
};

USTRUCT(BlueprintType, Category = "PRT|Struct")
struct FCEArgument
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|CE Argument")
	FString Name;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|CE Argument")
	int32 Type;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|CE Argument")
	float fValue;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|CE Argument")
	FString sValue;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "PRT|CE Argument")
	bool bValue;
};

USTRUCT(BlueprintType, Category = "PRT|Struct")
struct FCEAttribute
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="PRT|CE Attribute")
	FString DisplayName;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="PRT|CE Attribute")
	FString Name;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="PRT|CE Attribute")
	int32 Type;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="PRT|CE Attribute")
	float fValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="PRT|CE Attribute")
	FString sValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="PRT|CE Attribute")
	FLinearColor Color;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="PRT|CE Attribute")
	bool bValue;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PRT|CE Attribute")
	TArray<FCEArgument> Arguments;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="PRT|CE Attribute")
	bool showInVR = false;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PRT|CE Attribute")
	TArray<float> Range;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PRT|CE Attribute")
	float Step;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PRT|CE Attribute")
	float SliderStep;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PRT|CE Attribute")
	TArray<FString> SelectValues;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PRT|CE Attribute")
	bool bIsPercentage = false;
	bool bHidden = false;
	int32 Order = 0;

	//the type of widget to use:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="PRT|CE Attribute")
	ERPKWidgetTypes Widget = ERPKWidgetTypes::GENERAL_TEXT;
#if WITH_EDITOR
	//now we pack a class with all of the slate data and callbacks.
	FRPKWidget SlateWidget;
#endif
};

USTRUCT(BlueprintType, Category = "PRT|Struct")
struct FCEGroup
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="PRT|Group")
	FString Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="PRT|Group")
	TArray<FCEAttribute> Attributes;

	int32 Order;
};

USTRUCT(BlueprintType, Category = "PRT|Struct")
struct FCERPKViewAttributes
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="PRT|View Attributes")
	FString RPKFile;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="PRT|View Attributes")
	TArray<FCEGroup> ViewAttributes;
};

USTRUCT(BlueprintType, Category = "PRT|Struct")
struct FPRTReportMessage
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Log")
	FString Key;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Log")
	FString Value;
};

#pragma endregion


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), Category = "PRT")
class VITRUVIO_API APRTActor : public AActor
{
	GENERATED_BODY()

public:
	APRTActor();
	~APRTActor();

	// Sets default values for this actor's properties
	TMap<FString, FPRTAttribute> Attributes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Reporting")
	TArray<FPRTReportMessage> Reports;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Attributes")
	TArray<FRPKFile> RPKFiles;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Attributes")
	TArray<FOBJFile> OBJFiles;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Utility")
	bool bUseHardcodedValues = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Status")
	bool bGenerating = false;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Status")
	bool bMeshDataReady = false;

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|Attributes")
		bool IsGenerating();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Status")
	int32 GenerateCount;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Status")
	int32 GenerateSkipCount;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Status")
	float LastGenerationElapsedTime;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Status")
	float MeshingTime;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Status")
	float GenerateIdleTime;

	//UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PRT|Status")
	float MinimumTimeBetweenRegens = 1.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Status")
	int32 SectionCount = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Status")
	float StateManagerRuntime;

	// Read/Write so Actor or Server can set Dirty status, too.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Status")
		bool bAttributesUpdated = true; // Allow regen on first run

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Status")
	bool bHasEditor = false;

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|Status")
		bool InPIE();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|Status")
		bool InEditor();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|Status")
		bool InGame();

	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|Status")
	bool UsingGeneratorThread();
	
	///////////////////////////////////////////////


	//UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "PRT|Utility")
	TArray<FString> Rules;

	///////////////////////////////////////////////

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PRT|Components")
	UStaticMeshComponent* PRTStaticMesh;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PRT|Components")
	UProceduralMeshComponent* PRTProceduralMesh;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PRT|Components")
	UBoxComponent* PRTCollisionBox;

	///////////////////////////////////////////////

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Collision Box")
	float CollisionX_Scale = 0.5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Collision Box")
	float CollisionY_Scale = 0.5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Collision Box")
	float CollisionZ_Scale = 1.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Collision Box")
	float CollisionScale = 1.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Collision Box")
	float CollisionRotation = 0.0;

	UFUNCTION(BlueprintCallable, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|Collision Box")
	void SetCollisionBox(UBoxComponent* InCollisionBox) const;

	///////////////////////////////////////////////

	// RPK File Attribute arrays
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Attributes")
	TArray<FCEGroup> ViewAttributes;

	//the permanent Cache for Attributes.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Attributes")
	TArray<FCERPKViewAttributes> ViewAttributesDataStore;

	/*Copy the ViewAttributes from the current RPK and store them in ViewAttributesDataStore*/
	UFUNCTION(BlueprintCallable, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|Attributes")
	void CopyViewAttributesIntoDataStore();

	/*Retrieves the ViewAttributes from ViewAttributesDataStore for the current RPK */
	UFUNCTION(BlueprintCallable, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|Attributes")
	void RecallViewAttributes();

	///////////////////////////////////////////////

	/*attempt to load the RPK if we already have a file specified, this is mostly for use on opening a project,
	 *and restoring	the actor to the previous state.*/
	UFUNCTION(BlueprintCallable, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|Generate")
	void GetModelData(TArray<FPRTMeshStruct>& MeshStruct, bool& bDataValid);
	
	/* attempt to load the RPK if we already have a file specified, this is mostly for use on opening a project,
	 * and restoring	the actor to the previous state.
	 * Future: Create the Static Mesh using UE 4.24+ */
	UFUNCTION(BlueprintCallable, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|Generate")
	void GenerateModelData(bool bForceRegen = false);

	/////////////////////////////////////////////

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, CallInEditor, Category = "PRT|Events")
		void GenerateCompleted(bool Success);

	/////////////////////////////////////////////

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|RPK Settings")
		FString RPKPath = L"";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|RPK Settings")
		FString RPKFile = L"(none)";
	FString PreviousRPKFile = L""; //usable only by CopyViewAttributesIntoDataStore;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|RPK Settings")
		FString OBJPath = L"";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|RPK Settings")
		FString OBJFile = L"square";

	/*Sets the current RPK Path, initial shape and refreshes the building*/
	UFUNCTION(BlueprintCallable, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|RPK Settings")
		void InitializeRPKData(bool bCompleteReset);

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (HidePin = "Outer", DefaultToSelf = "Outer"), Category = "PRT|Options")
	bool bUseStaticMesh = false;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PRT|Options")
	EPRTLogVerbosity LogVerbosity = EPRTLogVerbosity::LOG_WARNING;
	
#pragma region Sync Data
	/*
	 *Store the specific string attribute from the current RPK into the Attributes Map before updating the building
	 *@param GroupIndex - index of ViewAttributes
	 *@param AttributeIndex - index of Attributes inside of ViewAttributes
	 *@param Value - The new string value to be used in generating the building
	 **/
	UFUNCTION(BlueprintCallable, Category = "PRT|Sync Data")
	void SyncAttributeString(int32 GroupIndex, int32 AttributeIndex, FString Value);

	/*
	 *Store the specific float attribute from the current RPK into the Attributes Map before updating the building
	 *@param GroupIndex - index of ViewAttributes
	 *@param AttributeibuteIndex - index of Attributes inside of ViewAttributes
	 *@param Value - The new float value to be used in generating the building
	 **/
	UFUNCTION(BlueprintCallable, Category = "PRT|Sync Data")
	void SyncAttributeFloat(int32 GroupIndex, int32 AttributeIndex, float Value);

	/*
	 *Store the specific bool attribute from the current RPK into the Attributes Map before updating the building
	 *@param GroupIndex - index of ViewAttributes
	 *@param AttributeibuteIndex - index of Attributes inside of ViewAttributes
	 *@param Value - The new bool value to be used in generating the building
	 **/
	UFUNCTION(BlueprintCallable, Category = "PRT|Sync Data")
	void SyncAttributeBool(int32 GroupIndex, int32 AttributeIndex, bool bValue);

	/*
	 *Store the specific color attribute from the current RPK into the Attributes Map before updating the building
	 *@param GroupIndex - index of ViewAttributes
	 *@param AttributeibuteIndex - index of Attributes inside of ViewAttributes
	 *@param Value - The new color value to be used in generating the building
	 **/
	UFUNCTION(BlueprintCallable, Category = "PRT|Sync Data")
	void SyncAttributeColor(int32 GroupIndex, int32 AttributeIndex, FLinearColor Value);
#pragma endregion

	/*
	 *Creates the procedural mesh
	 *@note Full Function implemented in Blueprint RPKDecoderActor
	 *@note this only works with the following 3 options "BlueprintImplementableEvent, BlueprintCallable, CallInEditor"
	 *@note Which unfortunately "CallInEditor" exposes the button to the editor, which
	 *@note sometime in the future, we need to mask from the user.
	*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, CallInEditor, Category = "PRT|Events", meta = (HidePin =
		"Outer", DefaultToSelf = "Outer"))
	void Generate(bool bForceRegen = false);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "PRT|Attributes", meta = (HidePin = "Outer",
		DefaultToSelf = "Outer"))
	void ClearViewAttributesDataStoreCache();

	// Called every frame
	void Tick(float DeltaTime) override;
#if WITH_EDITOR
	FPrtDetail* PrtDetail = nullptr;
#endif

	FVitruvioModule PRT;

private:
	FGenerator* PRTGenerator = nullptr;
	FPRTLog PRTLog;
	FPRTUtilities PRTUtil;

	bool bInitialized = false;
	double LastGenerationTimestamp = 0;
	TArray<FMeshDescription*> MeshDescriptions;
	
	prt::Status GenerateLocally();

	void LogGenerateStatistics();
	
	/*Searches for RPK and OBJ files in the current working directory and populates a list of RPKS for later use*/
	auto BuildFileLists(bool bRescan = false) -> void;

	void GetObjFileList(IFileManager& FileManager, FString ContentDir);
	void GetRPKFileList(IFileManager& FileManager, FString ContentDir);

#pragma region Meshing
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PRT|Utility")
	TArray<FPRTMeshStruct> MeshStructureStore;
	
	void GetComponents();

	static void CopyMeshStructures(TArray<FPRTMeshStruct>& SourceMeshStruct,
	                               TArray<FPRTMeshStruct>& DestinationMeshStruct);
	void ProcessPRTVertexDataIntoMeshStruct(TArray<FPRTMeshStruct>& MeshStruct);
	static void SetMaterialMeshVertexColors(FPRTMeshStruct* MaterialMesh, TArray<float> Vertices,
	                                        FLinearColor TempColor);
	static void SetMaterialMeshNormals(FPRTMeshStruct* MaterialMesh, TArray<float> Normals);
	static void SetMaterialMeshUVs(FPRTMeshStruct* MaterialMesh, TArray<float> UVs);
	static void SetMaterialMeshIndices(FPRTMeshStruct* MaterialMesh, TArray<uint32_t>& Indices);
#pragma endregion

#pragma region Attributes

	void RefreshDetailPanel();
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
