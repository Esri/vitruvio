// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "VitruvioModule.h"
#include "prt/Status.h"
#include <iostream>
#include "MessageLog/Public/MessageLogModule.h"
#include "Interfaces/IPluginManager.h"
#include "PrtDetail.h"
#include <Logging/LogMacros.h>
#include "HAL/FileManager.h"
#include <thread>
#include <mutex>
#include "PRTActor.h"

#define LOCTEXT_NAMESPACE "FVitruvioModule"

IMPLEMENT_MODULE(FVitruvioModule, Vitruvio);

TMap<FString, FPRTAttribute> FVitruvioModule::Attributes;
prt::Status FVitruvioModule::PluginStatus;

#define LOG_VERBOSE true

void FVitruvioModule::StartupModule()
{
	PRTUtil.SetCurrentWorkingDirectoryToPlugin();
#if LOG_VERBOSE
	PRTLog.Message(TEXT(">>>>                             <<<<"));
	PRTLog.Message(TEXT(">>>> Initializing the PRT Plugin <<<<"));
#endif

	// Load prt
	const FString PrtLibPath = PRTUtil.GetPluginBaseDirectory() + L"/Binaries/Win64/com.esri.prt.core.dll";
	Dlls.Add(FPlatformProcess::GetDllHandle(*PrtLibPath));

	const FString LibPath = PRTUtil.GetWorkingDirectory() + TEXT("Source/ThirdParty/PRT/lib/Win64/Release");
	TArray<wchar_t*> PRTPluginsPaths;
	PRTPluginsPaths.Add(const_cast<wchar_t*>(*LibPath));

	// Performs global Procedural Runtime initialization. Called once.
	// TODO: Try/Catch and message - Catch is likely due to missing dlls
	PRTInitializerHandle = init(
		PRTPluginsPaths.GetData(),
		PRTPluginsPaths.Num(),
		static_cast<prt::LogLevel>(3),
		&FVitruvioModule::PluginStatus);
	
	if (PluginStatus != prt::STATUS_OK)
	{
		const FString Message = "Failed to init CityEngine PRT plugin. Status: " + *prt::getStatusDescription(FVitruvioModule::PluginStatus);
		PRTLog.Message(*Message, ELogVerbosity::Warning);
		PRTLog.Dialog.Box(Message, "Plugin Initialize Failure.");
	}

#if WITH_EDITOR
	InitializeSlateAttributePanel();
#endif
	
	PRTUtil.RestoreOriginalWorkingDirectory();
	
#if LOG_VERBOSE
	PRTLog.Message(TEXT(">> DLLs Loaded. PRT Initialized."));
#endif
}

/**
 * \brief 
 */
void FVitruvioModule::InitializeSlateAttributePanel()
{
	// TODO: Move Slate functions to the PRTActor
	
	//This is where the Slate attribute panel is initialized with the plugin. 

#if WITH_EDITOR
	//PRTLog.Message(TEXT("Initializing Slate Panel for Attributes."));
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	//This binds it to the PRTActor that exists within the scene.	
	PropertyModule.RegisterCustomClassLayout(
		"PRTActor", 
		FOnGetDetailCustomizationInstance::CreateStatic(&FPRTDetail::MakeInstance));
#endif
}

void FVitruvioModule::ShutdownModule()
{
	// TODO: Threading shutdown
	
	for (int32 i = 0; i < Dlls.Num(); i++)
	{
		FPlatformProcess::FreeDllHandle(Dlls[i]);
	}

	Dlls.Empty();
	// this frees the library from use in the engine, and allows for city engine to cleanly shut down.

	PRTInitializerHandle->destroy();
	PRTInitializerHandle = nullptr;
}

/**
 * \brief Use a Generate command to retrieve the Attributes
 * \return 
 */
auto FVitruvioModule::GenerateAttributeResult() -> prt::Status
{
	//create and fill Initial Shapes with our one Initial Shape
	TArray<const prt::InitialShape*> InitialShapes;
	InitialShapes.Add(InitialShape);

	//create the encoder:
	const wchar_t* Encoder = L"com.esri.prt.core.AttributeEvalEncoder";
	const prt::EncoderInfo* EncoderInfo = prt::createEncoderInfo(Encoder);
	const prt::AttributeMap* EncoderOptions = nullptr;
	EncoderInfo->createValidatedOptionsAndStates(nullptr, &EncoderOptions);

	IAttributeResult* AttributeResult = new IAttributeResult();
	
	// Generate and encode procedural models for a number of initial shapes.	
	GenerateStatus = generate(InitialShapes.GetData(), 
		InitialShapes.Num(), 
		nullptr, 
		&Encoder, 
		1, 
		&EncoderOptions, 
		AttributeResult, 
		Cache, 
		nullptr);

	if (GenerateStatus != prt::STATUS_OK)
	{
		PRTLog.Message(TEXT("PRT Generate has encountered a problem, Status: "),
		       prt::getStatusDescription(GenerateStatus), ELogVerbosity::Warning);
	}
#if LOG_VERBOSE	
	else
	{
		PRTLog.Message(TEXT("PRT Generate Attribute Results: "), prt::getStatusDescription(GenerateStatus));
	}
#endif	
	return GenerateStatus;
}

auto FVitruvioModule::GenerateModel(TMap<FString, FPRTAttribute> InAttributes) -> prt::Status
{
	ApplyAttributesToProceduralRuntime(InAttributes);
	return GenerateModel();
}

auto FVitruvioModule::GenerateModel() -> prt::Status
{
	bIsGenerating = true;
	bIsCompleted = false;

#if LOG_VERBOSE
	PRTLog.Message(L"FVitruvioModule::GenerateModel Called");
#endif

	if (PluginStatus != prt::STATUS_OK)
	{
		PRTLog.Message(TEXT(">> GenerateModel Cancelled. Plugin Status: "), getStatusDescription(PluginStatus), ELogVerbosity::Warning);
			return PluginStatus;
	}
		
	//SetCurrentWorkingDirectoryToPlugin();
	
	prt::Status Status = CreateInitialShape();
	if(prt::STATUS_OK != Status)
	{
		PRTLog.Message(TEXT(">> CreateInitialShape error. Status: "), getStatusDescription(Status), ELogVerbosity::Warning);
		return Status;
	}
	
	//Create and fill Initial Shapes array with our one Initial Shape.
	TArray<const prt::InitialShape*> InitialShapes;
	InitialShapes.Add(InitialShape);

	//Create the OBJ Encoder:
	const wchar_t* Encoder = L"com.esri.prt.codecs.OBJEncoder";

	//Set Encoder Options.
	const prt::EncoderInfo* EncoderInfo = prt::createEncoderInfo(Encoder);
	const prt::AttributeMap* EncoderOptions = nullptr;
	EncoderInfo->createValidatedOptionsAndStates(nullptr, &EncoderOptions);
	EncoderInfo->destroy();
	
	prt::AttributeMapBuilder* AttributeMapBuilder = 
		prt::AttributeMapBuilder::createFromAttributeMap(EncoderOptions);

	//print Encoder Options.
	size_t NumberOfKeys;
	const wchar_t* const* Keys = EncoderOptions->getKeys(&NumberOfKeys, nullptr);
	AttributeMapBuilder->setBool(L"triangulateMeshes", true);

	EncoderOptions = AttributeMapBuilder->createAttributeMap();
	AttributeMapBuilder->destroy();

	//create a fresh memory callback (Not the attribute one.)
	OBJCallbackResult = prt::MemoryOutputCallbacks::create();

	//generate:
	GenerateStatus = generate(
		InitialShapes.GetData(), 
		InitialShapes.Num(), 
		nullptr, 
		&Encoder, 1, 
		&EncoderOptions, 
		OBJCallbackResult, 
		Cache, 
		nullptr, 
		AttributeMap);

	if (GenerateStatus != prt::STATUS_OK)
	{
		PRTLog.Message(TEXT(">> FVitruvioModule::GenerateModel() has encountered a problem: "),
		       prt::getStatusDescription(GenerateStatus), ELogVerbosity::Warning);
		return GenerateStatus;
	}

#if LOG_VERBOSE
	PRTLog.Message(TEXT("Successfully generated .obj data. The callback block count is: "),
	       int32(OBJCallbackResult->getNumBlocks()));
	FString Message;
	for (auto i = 0; i < OBJCallbackResult->getNumBlocks(); i++)
	{
		Message = Message.Printf(TEXT(">> File %d, Name: %s"), int32(OBJCallbackResult->getBlockContentType(i)), OBJCallbackResult->getBlockName(i));
		PRTLog.Message(Message);
	}
#endif
	
	LoadRPKDataToMemory();
#if LOG_VERBOSE
	PRTLog.Message(TEXT(">>> No. of Jpeg Images: "), JpegFiles.Num());
	PRTLog.Message(TEXT(">>> No. of Obj Files:   "), ObjectFiles.Num());
	PRTLog.Message(TEXT(">>> No. of Materials:   "), Materials.Num());

	for (auto& i : VertexData)
	{
		PRTLog.Message(TEXT("Number of Procedural Meshes to be created: "), i.Value.MaterialIndices.Num());
	}
#endif

	//PRTLog.Message(TEXT("GenerateModel Complete."));
	return prt::STATUS_OK;
	bIsGenerating = false;
	bIsCompleted = true;
}

auto FVitruvioModule::LoadRPKDataToMemory() -> prt::Status
{
	size_t Size;
	//empty JpegFiles if any are in memory
	for (auto& i : JpegFiles)
	{
		free(JpegFiles[i.Key]);
	}
	JpegSizes.Empty();
	JpegFiles.Empty();
	ObjectFiles.Empty();
	Materials.Empty();

	//read all files fresh into memory.
	for (size_t i = 0; i < OBJCallbackResult->getNumBlocks(); ++i)
	{
		const uint8_t* Data = OBJCallbackResult->getBlock(i, &Size);
		FString BlockName = OBJCallbackResult->getBlockName(i);
		switch (OBJCallbackResult->getBlockContentType(i))
		{
		case 1: //obj file
			ObjectFiles.Add(BlockName);
			ObjectFiles[BlockName] = FString(static_cast<size_t>(Size), (const char*)Data);
			break;
		case 2: //mtl file
			MaterialFiles.Add(BlockName);
			MaterialFiles[BlockName] = FString(static_cast<size_t>(Size), (const char*)Data);
			break;
		case 3: //jpg file.
			JpegFiles.Add(BlockName);
			JpegFiles[BlockName] = static_cast<uint8_t*>(malloc(sizeof(uint8_t*) * Size));
			for (size_t j = 0; j < Size; ++j)
			{
				JpegFiles[BlockName][j] = Data[j];
			}
			JpegSizes.Add(BlockName);
			JpegSizes[BlockName] = sizeof(uint8_t*) * Size;
			break;
		default:
			break;
		}
	}
	
	prt::Status Status = CreateMaterialData();
	if(Status != prt::STATUS_OK)
	{
		PRTLog.Message(">> Error in LoadRPKDataToMemory: ", Status, ELogVerbosity::Warning);
	}
	
	CreateVertexData();
	OBJCallbackResult->destroy();
	return prt::STATUS_OK;
}

void FVitruvioModule::CreateVertexDataFaceLine(FVertData& OutData, const FString Line, const FString Material)
{
	TArray<FString> Values = PRTUtil.SplitString(Line.TrimStartAndEnd(), ' ');
	if (Values.Num() > 2)
	{
		for (int32 l = 0; l < Values.Num(); l++)
		{
			CreateVertexDataFace(OutData, Values[l], Material);
		}
	}
}

auto FVitruvioModule::CreateVertexDataFace(FVertData& OutData, const FString Value, const FString Material) const -> prt::Status
{
	int32 v = 0, u = 0, n = 0, i;
	TArray<FString> Parts = PRTUtil.SplitString(Value, '/');

	//we shift -1 for each index, because obj files start at 1, instead of 0.
	//UE4 needs to start Indices from 0.
	if (Parts.Num() == 3)
	{
		//v = ParseNumber(parts[0]) - 1 + OutData.padding;
		v = PRTUtil.ParseNumber(Parts[0]) - 1;
		u = PRTUtil.ParseNumber(Parts[1]) - 1;
		n = PRTUtil.ParseNumber(Parts[2]) - 1;
	}
	if (Parts.Num() == 2)
	{
		//v = PRTUtil.ParseNumber(parts[0]) - 1 + OutData.padding;
		v = PRTUtil.ParseNumber(Parts[0]) - 1;
		u = PRTUtil.ParseNumber(Parts[1]) - 1;
		n = -1;
	}
	if (Parts.Num() == 1)
	{
		//v = PRTUtil.ParseNumber(parts[0]) - 1 + OutData.padding;
		v = PRTUtil.ParseNumber(Parts[0]) - 1;
		u = -1;
		n = -1;
	}
	if (OutData.MaterialVertices.Contains(Material))
	{
		i = OutData.MaterialVertices[Material].Num() / 3;
	}
	else
	{
		OutData.MaterialVertices.Add(Material);
		OutData.MaterialNormals.Add(Material);
		OutData.MaterialUVs.Add(Material);
		OutData.MaterialIndices.Add(Material);
		i = 0;
	}

	//with this new method, all faces will use unique vertices regardless. This will hope prevent corruption and help keep it clean.
	if (v * 3 + 2 < OutData.Vertices.Num() && v > -1)
	{
		OutData.MaterialVertices[Material].Add(OutData.Vertices[v * 3]);
		OutData.MaterialVertices[Material].Add(OutData.Vertices[v * 3 + 1]);
		OutData.MaterialVertices[Material].Add(OutData.Vertices[v * 3 + 2]);
		if (u > -1 && u * 2 + 1 < OutData.UVs.Num())
		{
			OutData.MaterialUVs[Material].Add(OutData.UVs[u * 2]);
			OutData.MaterialUVs[Material].Add(OutData.UVs[u * 2 + 1]);
		}
		else
		{
			//if not push two empties.
			OutData.MaterialUVs[Material].Add(0.0);
			OutData.MaterialUVs[Material].Add(0.0);
		}
		if (n > -1 && n * 3 + 2 < OutData.Normals.Num())
		{
			OutData.MaterialNormals[Material].Add(OutData.Normals[n * 3]);
			OutData.MaterialNormals[Material].Add(OutData.Normals[n * 3 + 1]);
			OutData.MaterialNormals[Material].Add(OutData.Normals[n * 3 + 2]);
		}
		else
		{
			OutData.MaterialNormals[Material].Add(OutData.Vertices[v * 3]);
			OutData.MaterialNormals[Material].Add(OutData.Vertices[v * 3 + 1]);
			OutData.MaterialNormals[Material].Add(OutData.Vertices[v * 3 + 2]);
		}
		//push back our face to match this vertex.
		OutData.MaterialIndices[Material].Add(i);
	}
	else
	{
		const FString ErrorMessage = TEXT("FATAL ERROR PARSING OBJ: vertex not found. Vertex Count: ") + OutData.Vertices.Num();
		PRTLog.Message(ErrorMessage, ELogVerbosity::Warning);
		return prt::STATUS_BUFFER_TO_SMALL;
	}
	return prt::STATUS_OK;
}

void FVitruvioModule::EmptyVertexData()
{
	// Clean the Vertex, Normals, UVs, and MaterialVerts  array.
	for (auto& i : VertexData)
	{
		for (auto& j : i.Value.MaterialIndices)
		{
			j.Value.Empty();
		}
		i.Value.MaterialIndices.Empty();

		for (auto& j : i.Value.MaterialNormals)
		{
			j.Value.Empty();
		}
		i.Value.MaterialNormals.Empty();

		for (auto& j : i.Value.MaterialUVs)
		{
			j.Value.Empty();
		}
		i.Value.MaterialUVs.Empty();

		for (auto& j : i.Value.MaterialVertices)
		{
			j.Value.Empty();
		}

		i.Value.MaterialIndices.Empty();
		i.Value.Vertices.Empty();
		i.Value.Normals.Empty();
		i.Value.UVs.Empty();
	}
	VertexData.Empty();
}

void FVitruvioModule::CreateVertexData()
{
	EmptyVertexData();

	for (auto& i : ObjectFiles)
	{
		//setup state properties.
		FString CurrurrentMaterial = TEXT("default_material");
		FVertData OutData;

		int32 BufferCursor = 0, BufferSize = 0;
		const wchar_t* Buffer = *i.Value;

		FVertData::EVertStatus Status = FVertData::EVertStatus::READ_COMMAND;

		for (int32 j = 0; j < i.Value.Len(); ++j)
		{
			BufferSize = j - BufferCursor;
			if (Status != FVertData::EVertStatus::COMMENT)
			{
				if (i.Value[j] == '#')
				{
					Status = FVertData::EVertStatus::COMMENT;
				}
				if (Status == FVertData::EVertStatus::READ_COMMAND)
				{
					if (i.Value[j] == ' ')
					{
						if (BufferSize > 0)
						{
							if (PRTUtil.StringCompare(Buffer, L"f"))
							{
								Status = FVertData::EVertStatus::READ_FACE;
							}
							if (PRTUtil.StringCompare(Buffer, L"v"))
							{
								Status = FVertData::EVertStatus::READ_VERTEX;
							}
							if (PRTUtil.StringCompare(Buffer, L"vn"))
							{
								Status = FVertData::EVertStatus::READ_NORMAL;
							}
							if (PRTUtil.StringCompare(Buffer, L"vt"))
							{
								Status = FVertData::EVertStatus::READ_UV;
							}
							if (PRTUtil.StringCompare(Buffer, L"g"))
							{
								Status = FVertData::EVertStatus::READ_G;
							}
							if (PRTUtil.StringCompare(Buffer, L"s"))
							{
								Status = FVertData::EVertStatus::READ_S;
							}
							if (PRTUtil.StringCompare(Buffer, L"usemtl"))
							{
								Status = FVertData::EVertStatus::READ_MTL;
							}
						}
						Buffer = *i.Value;
						BufferCursor = j + 1;
						Buffer += BufferCursor;
					}
				}
				else
				{
					//a command has been instantiated
					if (i.Value[j] == ' ' || i.Value[j] == '/' || i.Value[j] == '\n')
					{
						if (BufferSize > 0)
						{
							switch (Status)
							{
							case FVertData::EVertStatus::READ_VERTEX:
								//OutData.padding = OutData.nPadding;
								OutData.Vertices.Add(PRTUtil.ParseNumber(Buffer, BufferSize));
								Buffer = *i.Value;
								BufferCursor = j + 1;
								Buffer += BufferCursor;
								break;
							case FVertData::EVertStatus::READ_NORMAL:
								OutData.Normals.Add(PRTUtil.ParseNumber(Buffer, BufferSize));
								Buffer = *i.Value;
								BufferCursor = j + 1;
								Buffer += BufferCursor;
								break;
							case FVertData::EVertStatus::READ_UV:
								OutData.UVs.Add(PRTUtil.ParseNumber(Buffer, BufferSize));
								Buffer = *i.Value;
								BufferCursor = j + 1;
								Buffer += BufferCursor;
								break;

							case FVertData::EVertStatus::READ_MTL:
								if (i.Value[j] == '\n')
								{
									CurrurrentMaterial = FString((size_t)BufferSize, Buffer).TrimStartAndEnd();
									Buffer = *i.Value;
									BufferCursor = j + 1;
									Buffer += BufferCursor;
								}
								break;
							case FVertData::EVertStatus::READ_FACE:
								if (i.Value[j] == '\n')
								{
									CreateVertexDataFaceLine(OutData, FString((size_t)BufferSize, Buffer), CurrurrentMaterial);
									Buffer = *i.Value;
									BufferCursor = j + 1;
									Buffer += BufferCursor;
								}
								break;
							default:
								break;
							}
						}
					}
				}
			}

			if (i.Value[j] == '\n')
			{
				Status = FVertData::EVertStatus::READ_COMMAND;
				Buffer = *i.Value;
				BufferCursor = j + 1;
				Buffer += BufferCursor;
			}
		}

#if LOG_VERBOSE
		PRTLog.Message(L"> Object File Conversion Report:");
		PRTLog.Message(TEXT("  >>  Normals:  "), OutData.Normals.Num());
		PRTLog.Message(TEXT("  >>  Vertices: "), OutData.Vertices.Num());
		PRTLog.Message(TEXT("  >>  UVs:      "), (OutData.UVs.Num() / 2));
#endif
		
		// remove the temps:
		OutData.Vertices.Empty();
		OutData.Normals.Empty();
		OutData.UVs.Empty();
		if (!VertexData.Contains(i.Key)) VertexData.Add(i.Key);
		VertexData[i.Key] = OutData;
	}

}

auto FVitruvioModule::CreateMaterialData() -> prt::Status
{
#if LOG_VERBOSE
	PRTLog.Message(TEXT("FVitruvioModule::CreateMaterialData"));
#endif
	
	FString _currentMaterial = "default_material";
	for (auto i : MaterialFiles)
	{
		PRTLog.Message(L"> Loading MTL File...");

		TArray<FString> Lines = PRTUtil.SplitString(MaterialFiles[i.Key], '\n');
		for (int32 j = 0; j < Lines.Num(); ++j)
		{
			FString Line = Lines[j];
			Line = Line.TrimStartAndEnd();
			int32 Split;
			if (Line.FindChar(' ', Split))
			{
				FString command = Line.Left(Split).TrimStartAndEnd();
				FString value = Line.RightChop(Split + 1);
				if (command.Compare(TEXT("newmtl")) == 0)
				{
					_currentMaterial = value;
				}
				if (command.Compare(TEXT("map_Kd")) == 0)
				{
					if (!Materials.Contains(_currentMaterial)) Materials.Add(_currentMaterial);
					Materials[_currentMaterial].FileName = value;
				}
				if (command.Compare(TEXT("illum")) == 0)
				{
					if (!Materials.Contains(_currentMaterial)) Materials.Add(_currentMaterial);
					Materials[_currentMaterial].illum = PRTUtil.ParseNumber(value);
				}
				if (command.Compare(TEXT("Ns")) == 0)
				{
					if (!Materials.Contains(_currentMaterial)) Materials.Add(_currentMaterial);
					Materials[_currentMaterial].Ns = PRTUtil.ParseNumber(value);
				}
				if (command.Compare(TEXT("Ni")) == 0)
				{
					if (!Materials.Contains(_currentMaterial)) Materials.Add(_currentMaterial);
					Materials[_currentMaterial].Ni = PRTUtil.ParseNumber(value);
				}
				if (command.Compare(TEXT("d")) == 0)
				{
					if (!Materials.Contains(_currentMaterial)) Materials.Add(_currentMaterial);
					Materials[_currentMaterial].d = PRTUtil.ParseNumber(value);
				}
				if (command.Compare(TEXT("Tf")) == 0)
				{
					TArray<FString> values = PRTUtil.SplitString(value, ' ');
					if (values.Num() == 3)
					{
						if (!Materials.Contains(_currentMaterial)) Materials.Add(_currentMaterial);
						Materials[_currentMaterial].Tf[0] = PRTUtil.ParseNumber(values[0]);
						Materials[_currentMaterial].Tf[1] = PRTUtil.ParseNumber(values[1]);
						Materials[_currentMaterial].Tf[2] = PRTUtil.ParseNumber(values[2]);
					}
				}
				if (command.Compare(TEXT("Ka")) == 0)
				{
					TArray<FString> values = PRTUtil.SplitString(value, ' ');
					if (values.Num() == 3)
					{
						if (!Materials.Contains(_currentMaterial)) Materials.Add(_currentMaterial);
						Materials[_currentMaterial].Ka[0] = PRTUtil.ParseNumber(values[0]);
						Materials[_currentMaterial].Ka[1] = PRTUtil.ParseNumber(values[1]);
						Materials[_currentMaterial].Ka[2] = PRTUtil.ParseNumber(values[2]);
					}
				}
				if (command.Compare(TEXT("Kd")) == 0)
				{
					TArray<FString> values = PRTUtil.SplitString(value, ' ');
					if (values.Num() == 3)
					{
						if (!Materials.Contains(_currentMaterial)) Materials.Add(_currentMaterial);
						Materials[_currentMaterial].Kd[0] = PRTUtil.ParseNumber(values[0]);
						Materials[_currentMaterial].Kd[1] = PRTUtil.ParseNumber(values[1]);
						Materials[_currentMaterial].Kd[2] = PRTUtil.ParseNumber(values[2]);
					}
				}
				if (command.Compare(TEXT("Ks")) == 0)
				{
					TArray<FString> values = PRTUtil.SplitString(value, ' ');
					if (values.Num() == 3)
					{
						if (!Materials.Contains(_currentMaterial)) Materials.Add(_currentMaterial);
						Materials[_currentMaterial].Ks[0] = PRTUtil.ParseNumber(values[0]);
						Materials[_currentMaterial].Ks[1] = PRTUtil.ParseNumber(values[1]);
						Materials[_currentMaterial].Ks[2] = PRTUtil.ParseNumber(values[2]);
					}
				}
			}
		}
	}
	return prt::STATUS_OK;
}

/**
 * \brief Set the RPK in CE
 * \param InRPKFile 
 * \return prt::Status
 */
auto FVitruvioModule::SetRPKFile(FString InRPKFile) -> prt::Status
{
	RPKStatus = prt::STATUS_RESOLVEMAP_PROVIDER_NOT_FOUND; //set it to this until everything gets loaded.
	
#if LOG_VERBOSE
	PRTLog.Message(TEXT("FVitruvioModule::SetRPKFile"));
#endif

	if (InRPKFile.Len() == 0)
	{
		PRTLog.Message("RPK File Name is zero length.", ELogVerbosity::Warning);
		return prt::STATUS_STRING_TRUNCATED;
	}

	if(PluginStatus != prt::STATUS_OK)
	{
		PRTLog.Message(TEXT("FVitruvioModule::InitializeRPK() Plugin Status error. Status: "), PluginStatus, ELogVerbosity::Warning);
		return PluginStatus;
	}
	
	//adjust the path of the RpkFile to a URI.

	//empty everything, because we're changing.
	DestroyAll();

	const FString FullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(
		*FPaths::ProjectContentDir());
	InRPKFile = *FullPath + InRPKFile;

	PRTLog.Message(L"Path supplied: ", FullPath);

	if (InRPKFile.Len() > 1)
	{
		for (int32 i = 0; i < InRPKFile.Len(); i++)
		{
			switch (InRPKFile[i])
			{
			case '\\':
				InRPKFile[i] = '/';
				break;
			case ' ':
				InRPKFile = InRPKFile.Left(i) + TEXT("%20") + InRPKFile.RightChop(i + 1);
				break;
			default:
				break;
			}
		}
		InRPKFile = L"file:///" + InRPKFile;
	}
	RPKFile = InRPKFile;

	//create the resolve map.
	PRTLog.Message(TEXT(">> Setting RPK File to : "), *RPKFile);

	ResolveMap = createResolveMap(*RPKFile, nullptr, &RPKStatus);
	if (RPKStatus != prt::STATUS_OK)
	{
		// TODO: Provide error correction if possible
		
		FString Message = Message.Printf(TEXT("RPK File %s Could not be loaded, Status: %hs"), *RPKFile,
		                                 prt::getStatusDescription(RPKStatus));
		
		// TODO: Not Thread Safe Operation
		PRTLog.Dialog.Box(Message, "RPK Error");	
		PRTLog.Message(Message);
	}
	else
	{
		//deal with the cache.
		if (Cache != nullptr)
		{
			Cache->flushAll();
		}
		else
		{
			//create the cache:
			Cache = prt::CacheObject::create(prt::CacheObject::CACHE_TYPE_DEFAULT);
		}
	}

	return prt::STATUS_OK;
}

void FVitruvioModule::DestroyAll()
{
	//erase attributes
	for (auto& i : Attributes)
	{
		i.Value.Arguments.Empty();
	}
	Attributes.Empty();
	if (AttributeMap != nullptr)
	{
		AttributeMap->destroy();
		AttributeMap = nullptr;
	}
	if (ResolveMap != nullptr)
	{
		ResolveMap->destroy();
		ResolveMap = nullptr;
	}
	if (InitialShape != nullptr)
	{
		InitialShape->destroy();
		InitialShape = nullptr;
	}
	if (RuleInformation != nullptr)
	{
		RuleInformation = nullptr;
	}

	OBJFile = L"";
	RPKFile = L"";
	RuleFile = L"";
	StartRule = nullptr;



	for (auto& i : VertexData)
	{
		for (auto& j : i.Value.MaterialIndices)
		{
			j.Value.Empty();
		}
		i.Value.MaterialIndices.Empty();
		for (auto& j : i.Value.MaterialNormals)
		{
			j.Value.Empty();
		}
		i.Value.MaterialNormals.Empty();
		for (auto& j : i.Value.MaterialUVs)
		{
			j.Value.Empty();
		}
		i.Value.MaterialUVs.Empty();
		for (auto& j : i.Value.MaterialVertices)
		{
			j.Value.Empty();
		}
		i.Value.MaterialIndices.Empty();
		i.Value.Vertices.Empty();
		i.Value.Normals.Empty();
		i.Value.UVs.Empty();
	}
	VertexData.Empty();

	ObjectFiles.Empty();
	Materials.Empty();
	JpegFiles.Empty();
	JpegSizes.Empty();
	Materials.Empty();
}

auto FVitruvioModule::SetRule(FString InRuleFile) -> prt::Status
{
	if (RPKStatus != prt::STATUS_OK)
	{
		PRTLog.Message(TEXT("FVitruvioModule::SetRule - PKStatus error: "), RPKStatus, ELogVerbosity::Warning);
		return RPKStatus;
	}
	
	RuleFile = InRuleFile;
	prt::Status RuleStatus;

	PRTLog.Message(TEXT("FVitruvioModule::SetRule: File: "), *RuleFile);

	const wchar_t* ResolveStr = ResolveMap->getString(*RuleFile, &RuleStatus);
	if (RuleStatus == prt::STATUS_OK)
	{
		// wcout << "SetRule: Rule file string: " << ResolveStr << endl;
		const prt::RuleFileInfo* RuleFileInfo = createRuleFileInfo(ResolveStr, Cache);
		for (size_t r = 0; r < RuleFileInfo->getNumRules(); r++)
		{
			if (RuleFileInfo->getRule(r)->getNumParameters() > 0) continue;
			for (size_t a = 0; a < RuleFileInfo->getRule(r)->getNumAnnotations(); a++)
			{
				if (!(wcscmp(RuleFileInfo->getRule(r)->getAnnotation(a)->getName(), L"@StartRule")))
				{
					StartRule = RuleFileInfo->getRule(r);
					break;
				}
			}
		}
	}
	else
	{
		PRTLog.Message(TEXT(">> FVitruvioModule::SetRule: There was a problem setting the rule file. "), ELogVerbosity::Warning);
		RPKStatus = prt::STATUS_NO_RULEFILE;
	}
	return RPKStatus;
}

void FVitruvioModule::ApplyAttributesToProceduralRuntime(TMap<FString, FPRTAttribute> InAttributes)
{
	PRTUtil.SetCurrentWorkingDirectoryToPlugin();
#if LOG_VERBOSE
	PRTLog.Message(TEXT("FVitruvioModule::ApplyAttributesToProceduralRuntime"));
#endif

	Attributes.Empty();
	Attributes = InAttributes;

	prt::AttributeMapBuilder* Builder = prt::AttributeMapBuilder::create();

	for (auto& Attribute : Attributes)
	{
		FString KeyName = Attribute.Value.KeyName;
		switch (Attribute.Value.Type)
		{
		case prt::AAT_BOOL:
			Builder->setBool(*KeyName, Attribute.Value.bValue);
			break;
		case prt::AAT_FLOAT:
			Builder->setFloat(*KeyName, Attribute.Value.fValue);
			break;
		case prt::AAT_STR:
			Builder->setString(*KeyName, *Attribute.Value.sValue);
			break;
		default: break;
		}
	}
	AttributeMap = Builder->createAttributeMapAndReset();

	PRTUtil.RestoreOriginalWorkingDirectory();
}

void FVitruvioModule::SetInitialShape(FString InObjFile)
{
	FString FullPath;
#if LOG_VERBOSE
	//PRTLog.Message(TEXT("FVitruvioModule::SetInitialShape: File: "), InObjFile);
#endif 

	if (InObjFile.Len() < 1)
	{
		PRTLog.Message(TEXT("FVitruvioModule::SetInitialShape: Using Default Square.obj"));
		const FString ContentDirectoryPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(
			*(IPluginManager::Get().FindPlugin("PRT")->GetContentDir()));
		InObjFile = *ContentDirectoryPath;
		InObjFile += L"/InitialShapes/Square.obj";
	}
	else
	{
		const FString ProjectContentDirectory = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(
			*FPaths::ProjectContentDir());
		InObjFile = *ProjectContentDirectory + InObjFile;
	}

	for (int32 i = 0; i < InObjFile.Len(); i++)
	{
		switch (InObjFile[i])
		{
		case '\\':
			InObjFile[i] = '/';
			break;
		case ' ':
			InObjFile = InObjFile.Left(i) + TEXT("%20") + InObjFile.RightChop(i + 1);
			break;
		default:
			break;
		}
	}
	OBJFile = L"file:///" + InObjFile;
	
#if LOG_VERBOSE
	PRTLog.Message(L"FVitruvioModule::SetInitialShape, File: %s", OBJFile);
#endif 
}

auto FVitruvioModule::CreateInitialShape() -> prt::Status
{
	if(RPKStatus != prt::STATUS_OK)
	{
		PRTLog.Message(TEXT("FVitruvioModule::CreateInitialShape RPK Status error. Status: "), RPKStatus, ELogVerbosity::Warning);
		return RPKStatus;
	}

	if (StartRule == nullptr)
	{
		PRTLog.Message(TEXT("FVitruvioModule::CreateInitialShape. Start Rule == nullptr."), ELogVerbosity::Warning);
		return prt::STATUS_NO_RULEFILE;
	}

	//make sure attrMap is not nullptr. If so, build.
	if (AttributeMap == nullptr)
	{
		//PRTLog.Message(TEXT("FVitruvioModule::CreateInitialShape: Creating Attribute Map."));
		prt::AttributeMapBuilder* Builder = prt::AttributeMapBuilder::create();
		AttributeMap = Builder->createAttributeMapAndReset();
	}

	//destroy the initial shape if it exists.
	if (InitialShape != nullptr)
	{
		InitialShape->destroy();
	}
	
	//build the initial shape.
	prt::InitialShapeBuilder* InitialShapeBuilder = prt::InitialShapeBuilder::create();
	InitialShapeBuilder->setAttributes(*RuleFile, StartRule->getName(), 0, L"Quad", AttributeMap, ResolveMap);

	const prt::Status GeometryStatus = InitialShapeBuilder->resolveGeometry(*OBJFile, ResolveMap, Cache);

	// TODO: maybe do something with the status. Such as if failed, default to a square.
	if (GeometryStatus != prt::STATUS_OK)
	{
		PRTLog.Message(TEXT(">> FVitruvioModule::CreateInitialShape error. Status: "), GeometryStatus, ELogVerbosity::Warning);
	}

	InitialShape = InitialShapeBuilder->createInitialShapeAndReset();
	
#if LOG_VERBOSE
	PRTLog.Message(TEXT("FVitruvioModule::CreateInitialShape. Status: "), GeometryStatus);
#endif
	
	return GeometryStatus;
}

TArray<FString> FVitruvioModule::GetRules()
{
#if LOG_VERBOSE
	PRTLog.Message(TEXT("FVitruvioModule::GetRules"));
#endif

	TArray<FString> Rules;
	if (RPKStatus == prt::STATUS_OK)
	{
		PRTUtil.SetCurrentWorkingDirectoryToPlugin();

		size_t Count, NumberOfRules = 0;
		const FString FileExtension = L".cgb";
		const wchar_t* const* Keys = ResolveMap->getKeys(&Count);
		for (size_t i = 0; i < Count; i++)
		{
			FString Key = Keys[i];
			if (FileExtension.Compare(Key.Mid(Key.Len() - 4, 4)) == 0)
			{
				Rules.Add(Key);
				++NumberOfRules;
			}
		}

		if (NumberOfRules < 1)
		{
			RPKStatus = prt::STATUS_NO_RULEFILE;
			PRTLog.Message(L">> ERROR: ", RPKStatus, ELogVerbosity::Warning);
		}

		PRTUtil.RestoreOriginalWorkingDirectory();

		PRTLog.Message(TEXT("Rule File Count: "), int32(NumberOfRules));
	}
	else
	{
		PRTLog.Message(TEXT(">> FVitruvioModule::GetRules. PRKStatus: "), RPKStatus, ELogVerbosity::Warning);
	}
	return Rules;
}

TMap<FString, FPRTAttribute> FVitruvioModule::GetAttributes()
{
#if LOG_VERBOSE
	PRTLog.Message(TEXT("FVitruvioModule::GetAttributes"));
#endif

	if (RPKStatus == prt::STATUS_OK)
	{
		CreateInitialShape(); //builds the initial shape for the generate command.
		GenerateAttributeResult(); //Fills the attributes array
		
#if LOG_VERBOSE
		PRTLog.Message(TEXT(">> Using Rule: "), *RuleFile);
#endif 

		prt::Status RuleStatus;
		RuleInformation = createRuleFileInfo(ResolveMap->getString(*RuleFile), Cache, &RuleStatus);

		if (RuleStatus != prt::STATUS_OK)
		{
			PRTLog.Message(TEXT(">> createRuleFileInfo error in GetAttributes. Status: "), RuleStatus, ELogVerbosity::Warning);
			PRTUtil.RestoreOriginalWorkingDirectory();

			RPKStatus = prt::STATUS_UNKNOWN_RULE;
			return Attributes;
		}
#if LOG_VERBOSE
		PRTLog.Message(TEXT(">> Attributes: "),  int32(RuleInformation->getNumAttributes()));
		PRTLog.Message(TEXT(">> Annotations: "), int32(RuleInformation->getNumAnnotations()));
#endif 

		for (size_t i = 0; i < RuleInformation->getNumAttributes(); ++i)
		{
			const prt::RuleFileInfo::Entry* Attribute = RuleInformation->getAttribute(i);

			for (size_t j = 0; j < Attribute->getNumAnnotations(); ++j)
			{
				const prt::Annotation* Annotation = Attribute->getAnnotation(j);

				size_t NumberOfArguments = Annotation->getNumArguments();
				if (NumberOfArguments > 0)
				{
					for (size_t k = 0; k < NumberOfArguments; ++k)
					{
						const prt::AnnotationArgument* Argument = Annotation->getArgument(k);
						FVitruvioModuleArgument PreviousArgument;
						PreviousArgument.Type = Argument->getType();
						PreviousArgument.bValue = Argument->getBool();
						PreviousArgument.fValue = Argument->getFloat();
						PreviousArgument.sValue = Argument->getStr();
						PreviousArgument.KeyName = Annotation->getName();
						if (!Attributes.Contains(Attribute->getName())) Attributes.Add(Attribute->getName());
						Attributes[Attribute->getName()].Arguments.Add(PreviousArgument);
					}
				}
				else
				{
					FVitruvioModuleArgument PreviousArgument;
					PreviousArgument.Type = prt::AAT_BOOL;
					PreviousArgument.bValue = true;
					PreviousArgument.sValue = L"";
					PreviousArgument.fValue = 0.0f;
					PreviousArgument.KeyName = Annotation->getName();
					if (!Attributes.Contains(Attribute->getName())) Attributes.Add(Attribute->getName());
					Attributes[Attribute->getName()].Arguments.Add(PreviousArgument);
				}
			}
		}
	}
	else
	{
		PRTLog.Message(TEXT(">> RPK Status error in FVitruvioModule::GetAttributes. Status: "), RPKStatus, ELogVerbosity::Warning);
	}
	return Attributes;
}

#pragma region IAttributeResult

auto IAttributeResult::attrBool(size_t IsIndex, int32_t ShapeID, const wchar_t* Key, bool Value) -> prt::Status
{
	if (!FVitruvioModule::Attributes.Contains(Key)) FVitruvioModule::Attributes.Add(Key);
	FVitruvioModule::Attributes[Key].KeyName = Key;
	FVitruvioModule::Attributes[Key].bValue = Value;
	FVitruvioModule::Attributes[Key].Type = prt::AAT_BOOL;
	return prt::STATUS_OK;
}

auto IAttributeResult::attrFloat(size_t IsIndex, int32_t ShapeID, const wchar_t* Key, double Value) -> prt::Status
{
	if (!FVitruvioModule::Attributes.Contains(Key)) FVitruvioModule::Attributes.Add(Key);
	FVitruvioModule::Attributes[Key].KeyName = Key;
	FVitruvioModule::Attributes[Key].fValue = Value;
	FVitruvioModule::Attributes[Key].Type = prt::AAT_FLOAT;
	return prt::STATUS_OK;
}

auto IAttributeResult::attrString(size_t IsIndex, int32_t ShapeID, const wchar_t* Key,
                                  const wchar_t* Value) -> prt::Status
{
	if (!FVitruvioModule::Attributes.Contains(Key)) FVitruvioModule::Attributes.Add(Key);
	FVitruvioModule::Attributes[Key].KeyName = Key;
	FVitruvioModule::Attributes[Key].sValue = Value;
	FVitruvioModule::Attributes[Key].Type = prt::AAT_STR;
	return prt::STATUS_OK;
}

prt::Status IAttributeResult::attrBoolArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const bool* ptr, size_t size)
{
	// TODO: Implement
	return prt::Status();
}

prt::Status IAttributeResult::attrFloatArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const double* ptr, size_t size)
{
	// TODO: Implement
	return prt::Status();
}

prt::Status IAttributeResult::attrStringArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* const* ptr, size_t size)
{
	// TODO: Implement
	return prt::Status();
}

auto IAttributeResult::generateError(size_t isIndex, prt::Status status, const wchar_t* message) -> prt::Status
{
	// TODO: Implement
	return prt::STATUS_OK;
};

auto IAttributeResult::assetError(size_t isIndex, prt::CGAErrorLevel level, const wchar_t* key,
                                  const wchar_t* uri, const wchar_t* message) -> prt::Status 
{ 
	// TODO: Implement
	return prt::STATUS_OK; 
};

auto IAttributeResult::cgaError(size_t isIndex, int32_t shapeID, prt::CGAErrorLevel level, int32_t methodId,
                                int32_t pc, const wchar_t* message) -> prt::Status 
{ 
	// TODO: Implement
	return prt::STATUS_OK; 
};

auto IAttributeResult::cgaPrint(size_t isIndex, int32_t shapeID, const wchar_t* txt) -> prt::Status
{
	// TODO: Implement
	return prt::STATUS_OK;
};

auto IAttributeResult::cgaReportBool(size_t isIndex, int32_t shapeID, const wchar_t* key, bool value) -> prt::Status
{
	// TODO: Implement
	return prt::STATUS_OK;
};

auto IAttributeResult::cgaReportFloat(size_t isIndex, int32_t shapeID, const wchar_t* key, double value) -> prt::Status
{
	// TODO: Implement
	return prt::STATUS_OK;
};

prt::Status IAttributeResult::cgaReportString(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* value)
{
	// TODO: Implement
	return prt::STATUS_OK;
};

#pragma endregion IAttributeResult 

/*
void FVitruvioModule::UnitTest1()
{
	//This is an example to how to build the input options for the conversion.
	//start by setting the RPK.
	SetRPKFile(L"D:/Workspace/esri-cityengine-sdk/data/candler.rpk");

	//then get a list of rules
	TArray<FString> rules = GetRules();

	SetRule(rules[0]);

	//push the initial shape to the plugin, we need to push it before getting attributes.
	SetInitialShape(L"./Content/InitialShapes/square.obj");

	//get attributes
	TMap<FString, FPRTAttribute> attributes = GetAttributes();

	//alter Attributes:
	attributes[L"Default$FloorHeight"].fValue = 10.0;

	//send the attributes back and go.
	ApplyAttributesToProceduralRuntime(attributes);

	//We need to push the initial shape again to set the new attributes with the initial shape.
	SetInitialShape(L"./Content/InitialShapes/square.obj");

	//finally, let's get the generated rpk:
	GenerateModel(); //build the initial obj file from the RPK.

	for (int32 i = 0; i < 1; i++)
	{
		//let's do it once more to see if it can be hammered.
		//get attributes
		attributes = GetAttributes();

		//alter Attributes:
		attributes[L"Default$FloorHeight"].fValue = 20.0;

		//send the attributes back and go.
		ApplyAttributesToProceduralRuntime(attributes);

		//finally, let's get the generated rpk:
		GenerateModel(); //build the initial obj file from the RPK.
	}
}
*/


bool FVitruvioModule::IsLoaded()
{
	if (PluginStatus != prt::STATUS_OK)
	{
		PRTLog.Message(TEXT("FVitruvioModule::IsLoaded() Plugin Status = "), PluginStatus, ELogVerbosity::Warning);

		if (RPKStatus != prt::STATUS_OK)
		{
			PRTLog.Message(TEXT("FVitruvioModule::IsLoaded() RPK Plugin Status = "), RPKStatus, ELogVerbosity::Warning);
		}
		return false;
	}
	return true;
}

bool FVitruvioModule::IsGenerating()
{
	return bIsGenerating;
}


bool FVitruvioModule::IsDone()
{
	return bIsCompleted;
}


#undef LOCTEXT_NAMESPACE
