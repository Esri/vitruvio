// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include <locale>
#include <codecvt>
#include <stdint.h>

#include <Engine.h>

//City Engine
//#include "prt/prt.h"
#include "prt/API.h"
//#include "prt/ContentType.h"
#include "prt/Callbacks.h"
#include "prt/MemoryOutputCallbacks.h"
#include "prt/Cache.h"

#include "prtx/Exception.h"
//#include "prtx/Log.h"
//#include "prtx/Geometry.h"
//#include "prtx/Material.h"
#include "prtx/Shape.h"
//#include "prtx/ShapeIterator.h"
#include "prtx/ExtensionManager.h"
//#include "prtx/GenerateContext.h"

//UE4 modules
#include "Windows/MinWindows.h"
//#include <Shlwapi.h>
#include "Modules/ModuleManager.h"
#include "PRTLog.h"
#include "PRTUtilities.h"

#if WITH_EDITOR
#include "PropertyEditorModule.h"
#endif

//#include "MessageLog.h"

class APRTActor;
using namespace std;

#pragma region Structure and Enum

/*
struct PRTOptions {
	string encoder = "com.esri.prt.codecs.OBJEncoder"; //mEncoderID
	vector < array <string, 3> > encoderOptions; //convertEncOpts
	vector< array <string, 3> > attributes; //convertShapeAttrs
	//possibility: a flag to output to disk or callback.
};
*/
struct FMatData
{
	FString FileName = L"";
	float Kd[3] = {1.0f, 1.0f, 1.0f};
	float Ka[3] = {1.0f, 1.0f, 1.0f};
	float Ks[3] = {1.0f, 1.0f, 1.0f};
	int32 illum = 0;
	int32 Ns = 0;
	int32 d = 0;
	BYTE Tf[3];
	float Ni = 1.0;
};

struct FVertData
{
	TArray<float> Vertices;
	TArray<float> Normals;
	TArray<float> UVs; // UVs specifically ordered for UE4
	TMap<FString, TArray<float>> MaterialVertices;
	TMap<FString, TArray<float>> MaterialNormals;
	TMap<FString, TArray<float>> MaterialUVs;
	TMap<FString, TArray<uint32_t>> MaterialIndices;

	//State Machine for parsing / loading obj.
	enum EVertStatus : uint8
	{
		READ_COMMAND,
		COMMENT,
		READ_VERTEX,
		READ_FACE,
		READ_NORMAL,
		READ_UV,
		READ_G,
		READ_S,
		READ_MTL
	};
};

struct FPRTModuleArgument
{
	FString KeyName;
	int32 Type;
	bool bValue;
	float fValue;
	FString sValue;
};

struct FPRTAttribute
{
	FString KeyName;
	int32 Type;
	bool bValue;
	float fValue;
	FString sValue;
	TArray<FPRTModuleArgument> Arguments;
};

class IAttributeResult : public prt::Callbacks
{
public:
	prt::Status generateError(size_t IsIndex, prt::Status Status, const wchar_t* Message) override;
	prt::Status assetError(size_t IsIndex, prt::CGAErrorLevel Level, const wchar_t* Key, const wchar_t* uri,
	                       const wchar_t* Message) override;
	prt::Status cgaError(size_t IsIndex, int32_t ShapeID, prt::CGAErrorLevel Level, int32_t MethodId, int32_t pc,
	                     const wchar_t* Message) override;
	prt::Status cgaPrint(size_t IsIndex, int32_t ShapeID, const wchar_t* Txt) override;
	prt::Status cgaReportBool(size_t IsIndex, int32_t ShapeID, const wchar_t* Key, bool Value) override;
	prt::Status cgaReportFloat(size_t IsIndex, int32_t ShapeID, const wchar_t* Key, double Value) override;
	prt::Status cgaReportString(size_t IsIndex, int32_t ShapeID, const wchar_t* Key, const wchar_t* Value) override;

	prt::Status attrBool(size_t isIndex, int32_t ShapeID, const wchar_t* Key, bool Value) override;
	prt::Status attrFloat(size_t IsIndex, int32_t ShapeID, const wchar_t* Key, double Value) override;
	prt::Status attrString(size_t IsIndex, int32_t ShapeID, const wchar_t* Key, const wchar_t* Value) override;

	// TODO: Not implemented in PRT Plugin
	prt::Status attrBoolArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const bool* ptr, size_t size) override;
	prt::Status attrFloatArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const double* ptr, size_t size) override;
	prt::Status attrStringArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* const* ptr, size_t size) override;
};

#pragma endregion

/*
#define LOCTEXT_NAMESPACE "EpicWrenNamespace"
	extern FName GLoggerName;
	extern FMessageLog GLogger;
*/

class IPRTModule : public IModuleInterface
{
public:

	virtual auto SetRPKFile(FString InRpkFilename)->prt::Status =0; //step 1
	virtual TArray<FString> GetRules() = 0; //step 2
	virtual auto SetRule(FString RuleFile)->prt::Status =0;
	virtual TMap<FString, FPRTAttribute> GetAttributes() =0; //step 3
	//virtual void ApplyAttributesToProceduralRuntime(TMap<FString, FPRTAttribute> Attributes)=0;
	virtual void SetInitialShape(FString InObjFilename)=0;
	virtual auto GenerateModel(TMap<FString, FPRTAttribute> Attributes)->prt::Status =0;
	virtual auto GenerateModel()->prt::Status = 0;
	
	virtual void InitializeSlateAttributePanel() =0;
	virtual auto IsLoaded() -> bool =0;

	virtual auto IsGenerating() -> bool =0;
	virtual auto IsDone() -> bool =0;

	virtual auto ApplyAttributesToProceduralRuntime(TMap<FString, FPRTAttribute> Attributes) -> void =0;
	
	/*static TMap<FString, FPRTAttribute> Attributes;
	TMap<FString, FVertData> VertexData;
	TMap<FString, FString> ObjectFiles;
	TMap<FString, FString> MaterialFiles;
	TMap<FString, uint8_t*> JpegFiles;
	TMap<FString, int32> JpegSizes;
	TMap<FString, FMatData> Materials;*/


	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IPRTModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IPRTModule >("PRTPluginModule");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("PRTPluginModule");
	}
};


class FPRTModule : public IPRTModule
{
	friend APRTActor;


public:
	/** IModuleInterface implementation */
	void StartupModule() override;
	void ShutdownModule() override;

	auto SetRPKFile(FString InRpkFilename)->prt::Status override; //step 1
	TArray<FString> GetRules() override; //step 2
	auto SetRule(FString RuleFile)->prt::Status override;
	TMap<FString, FPRTAttribute> GetAttributes() override; //step 3

	void SetInitialShape(FString InObjFilename) override;
	auto GenerateModel() -> prt::Status override;
	auto GenerateModel(TMap<FString, FPRTAttribute> Attributes)->prt::Status override;
	
	void InitializeSlateAttributePanel() override;

	static TMap<FString, FPRTAttribute> Attributes;
	TMap<FString, FVertData> VertexData;
	TMap<FString, FString> ObjectFiles;
	TMap<FString, FString> MaterialFiles;
	TMap<FString, uint8_t *> JpegFiles;
	TMap<FString, int32> JpegSizes;
	TMap<FString, FMatData> Materials;
	bool IsLoaded() override;

	auto IsGenerating() -> bool override;
	auto IsDone() -> bool override;

	auto ApplyAttributesToProceduralRuntime(TMap<FString, FPRTAttribute> Attributes) -> void override;
private:


	//utility methods
	TArray<void*> Dlls;

	bool bIsGenerating;
	bool bIsCompleted;
	
	FPRTLog PRTLog;
	////Stuff for the dialog and printing information to the user.
	//struct FDialog
	//{
	//	FText Body;
	//	FText Title;
	//	void Box(FString Body, FString Title);
	//} Dialog;

	/* PRT Plugin */
	static prt::Status PluginStatus; //This tells the rest of the plugin if the plugin is in an okay state and usable.
	prt::Status RPKStatus = prt::STATUS_UNSPECIFIED_ERROR; //the status of the rpk
	prt::Status GenerateStatus = prt::STATUS_UNSPECIFIED_ERROR; //the status if a generate has occurred.
	const prt::RuleFileInfo* RuleInformation = nullptr;
	const prt::Object* PRTInitializerHandle = nullptr; //The initializer handle.

	FString RPKFile;
	FString OBJFile;
	const prt::ResolveMap* ResolveMap = nullptr;
	prt::Cache* Cache = nullptr;
	const prt::InitialShape* InitialShape = nullptr;

	prt::MemoryOutputCallbacks* OBJCallbackResult = nullptr;
	
	//encoder information.
	auto GenerateAttributeResult() -> prt::Status; //Fills the attributes array
	auto Generate() -> prt::Status;
	auto LoadRPKDataToMemory() -> prt::Status;
	auto CreateInitialShape() -> prt::Status;
	void CreateVertexData();
	void CreateVertexDataFaceLine(FVertData& OutData, FString Line, FString Material);
	auto CreateVertexDataFace(FVertData& OutData, FString Value, FString Material) const -> prt::Status;
	void EmptyVertexData();
	auto CreateMaterialData() -> prt::Status;
	void DestroyAll();

	//rule info
	FString RuleFile;
	const prt::RuleFileInfo::Entry* StartRule = nullptr;
	
	//utility methods
	FPRTUtilities PRTUtil;

	//Unit Tests:
	void UnitTest1();
	const prt::AttributeMap* AttributeMap = nullptr;
};
