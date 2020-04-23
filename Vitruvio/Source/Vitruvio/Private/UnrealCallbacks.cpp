#pragma once

#include "UnrealCallbacks.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialExpressionConstant.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshDescription.h"
#include "StaticMeshOperations.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Materials/MaterialInstanceDynamic.h"

#include <map>
#include <string>

DEFINE_LOG_CATEGORY(LogUnrealCallbacks);

namespace
{
	FPlane GetRow(const double* Mat, int32 Index)
	{
		return FPlane(Mat[Index + 0 * 4], Mat[Index + 1 * 4], Mat[Index + 2 * 4], Mat[Index + 3 * 4]);
	}

	UTexture2D* CreateTexture(UObject* Outer, const TArray<uint8>& PixelData, int32 InSizeX, int32 InSizeY, EPixelFormat InFormat, FName BaseName)
	{
		// Shamelessly copied from UTexture2D::CreateTransient with a few modifications
		if (InSizeX <= 0 || InSizeY <= 0 ||
			(InSizeX % GPixelFormats[InFormat].BlockSizeX) != 0 ||
			(InSizeY % GPixelFormats[InFormat].BlockSizeY) != 0)
		{
			UE_LOG(LogUnrealCallbacks, Warning, TEXT("Invalid parameters"));
			return nullptr;
		}

		// Most important difference with UTexture2D::CreateTransient: we provide the new texture with a name and an owner
		const FName TextureName = MakeUniqueObjectName(Outer, UTexture2D::StaticClass(), BaseName);
		UTexture2D* NewTexture = NewObject<UTexture2D>(Outer, TextureName, RF_Transient);

		NewTexture->PlatformData = new FTexturePlatformData();
		NewTexture->PlatformData->SizeX = InSizeX;
		NewTexture->PlatformData->SizeY = InSizeY;
		NewTexture->PlatformData->PixelFormat = InFormat;

		// Allocate first mipmap and upload the pixel data
		const int32 NumBlocksX = InSizeX / GPixelFormats[InFormat].BlockSizeX;
		const int32 NumBlocksY = InSizeY / GPixelFormats[InFormat].BlockSizeY;
		FTexture2DMipMap* Mip = new(NewTexture->PlatformData->Mips) FTexture2DMipMap();
		Mip->SizeX = InSizeX;
		Mip->SizeY = InSizeY;
		Mip->BulkData.Lock(LOCK_READ_WRITE);
		void* TextureData = Mip->BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[InFormat].BlockBytes);
		FMemory::Memcpy(TextureData, PixelData.GetData(), PixelData.Num());
		Mip->BulkData.Unlock();

		NewTexture->UpdateResource();
		return NewTexture;
	}

	UTexture2D* LoadImageFromDisk(UObject* Outer, const FString& ImagePath)
	{
		static IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		
		if (!FPaths::FileExists(ImagePath))
		{
			UE_LOG(LogUnrealCallbacks, Error,  TEXT("File not found: %s"), *ImagePath);
			return nullptr;
		}

		TArray<uint8> FileData;
		if (!FFileHelper::LoadFileToArray(FileData, *ImagePath))
		{
			UE_LOG(LogUnrealCallbacks, Error, TEXT("Failed to load file: %s"), *ImagePath);
			return nullptr;
		}
		const EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(FileData.GetData(), FileData.Num());
		if (ImageFormat == EImageFormat::Invalid)
		{
			UE_LOG(LogUnrealCallbacks, Error, TEXT("Unrecognized image file format: %s"), *ImagePath);
			return nullptr;
		}

		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
		if (!ImageWrapper.IsValid())
		{
			UE_LOG(LogUnrealCallbacks, Error, TEXT("Failed to create image wrapper for file: %s"), *ImagePath);
			return nullptr;
		}

		// Decompress the image data
		TArray<uint8> RawData;
		ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num());
		ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData);

		// Create the texture and upload the uncompressed image data
		const FString TextureBaseName = TEXT("T_") + FPaths::GetBaseFilename(ImagePath);
		return CreateTexture(Outer, RawData, ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), EPixelFormat::PF_B8G8R8A8, FName(*TextureBaseName));
	}

	UTexture2D* GetTexture(UObject* Outer, const prt::AttributeMap* MaterialAttributes, wchar_t const* Key)
	{
		size_t ValuesCount = 0;
		wchar_t const* const* Values = MaterialAttributes->getStringArray(Key, &ValuesCount);
		for (int ValueIndex = 0; ValueIndex < ValuesCount; ++ValueIndex)
		{
			std::wstring TextureUri(Values[ValueIndex]);
			if (TextureUri.size() > 0)
			{
				return LoadImageFromDisk(Outer, FString(Values[ValueIndex]));
			}
		}
		return nullptr;
	}

	FLinearColor GetLinearColor(const prt::AttributeMap* MaterialAttributes, wchar_t const* Key)
	{
		size_t count; 
		const double* values = MaterialAttributes->getFloatArray(Key, &count);
		if (count < 3)
		{
			return FLinearColor();
		}
		const FColor Color(values[0] * 255.0, values[1] * 255.0, values[2] * 255.0);
		return FLinearColor(Color);
	}

	float GetScalar(const prt::AttributeMap* MaterialAttributes, wchar_t const* Key)
	{
		return MaterialAttributes->getFloat(Key);
	}
	
	enum MaterialPropertyType
	{
		TEXTURE, LINEAR_COLOR, SCALAR
	};

	// see prtx/Material.h
	const std::map<std::wstring, MaterialPropertyType> KeyToTypeMap = {
		{L"diffuseMap", 	MaterialPropertyType::TEXTURE},
		{L"opacityMap", 	MaterialPropertyType::TEXTURE},
		{L"emissiveMap", 	MaterialPropertyType::TEXTURE},
		{L"metallicMap", 	MaterialPropertyType::TEXTURE},
		{L"roughnessMap", 	MaterialPropertyType::TEXTURE},
		{L"normalMap", 		MaterialPropertyType::TEXTURE},
		
		{L"diffuseColor", 	MaterialPropertyType::LINEAR_COLOR},
		{L"emissiveColor", 	MaterialPropertyType::LINEAR_COLOR},
		
		{L"metallic", 	MaterialPropertyType::SCALAR},
		{L"opacity", 	MaterialPropertyType::SCALAR},
		{L"roughness", 	MaterialPropertyType::SCALAR},
	};

	UMaterialInstanceDynamic* CreateMaterial(UObject* Outer, UMaterialInterface* Parent, const prt::AttributeMap* MaterialAttributes)
	{
		UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(Parent, Outer);

		// Convert AttributeMap to material
		size_t KeyCount = 0;
		wchar_t const* const* Keys = MaterialAttributes->getKeys(&KeyCount);
		for (int KeyIndex = 0; KeyIndex < KeyCount; KeyIndex++) 
		{
			const wchar_t* Key = Keys[KeyIndex];
			const std::wstring KeyString(Keys[KeyIndex]);
			const auto TypeIter = KeyToTypeMap.find(KeyString);
			if (TypeIter != KeyToTypeMap.end())
			{
				const MaterialPropertyType Type = TypeIter->second;
				switch (Type)
				{
				// TODO loading the texture should probably not happen in the game thread
				case TEXTURE:
					MaterialInstance->SetTextureParameterValue(FName(Key), GetTexture(Outer, MaterialAttributes, Key));
					break;
				case LINEAR_COLOR:
					MaterialInstance->SetVectorParameterValue(FName(Key), GetLinearColor(MaterialAttributes, Key));
					break;
				case SCALAR:
					MaterialInstance->SetScalarParameterValue(FName(Key), GetScalar(MaterialAttributes, Key));
					break;
				default:;
				}
			}
		}

		return MaterialInstance;
	}
}

void UnrealCallbacks::addMesh(const wchar_t* name, int32_t prototypeId, const double* vtx, size_t vtxSize, const double* nrm, size_t nrmSize, const uint32_t* faceVertexCounts,
							  size_t faceVertexCountsSize, const uint32_t* vertexIndices, size_t vertexIndicesSize, const uint32_t* normalIndices, size_t normalIndicesSize,

							  double const* const* uvs, size_t const* uvsSizes, uint32_t const* const* uvCounts, size_t const* uvCountsSizes, uint32_t const* const* uvIndices,
							  size_t const* uvIndicesSizes, size_t uvSets,

							  const uint32_t* faceRanges, size_t faceRangesSize,
							  const prt::AttributeMap** materials)
{
	UStaticMesh* Mesh = NewObject<UStaticMesh>();

	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();

	if (uvSets > 8)
	{
		UE_LOG(LogUnrealCallbacks, Error, TEXT("Mesh %s uses %llu UV sets but only 8 are allowed. Clamping UV sets to 8."), name, uvSets);
		uvSets = 8;
	}

	const auto VertexUVs = Attributes.GetVertexInstanceUVs();
	Attributes.GetVertexInstanceUVs().SetNumIndices(uvSets);

	// Convert vertices and vertex instances
	const auto VertexPositions = Attributes.GetVertexPositions();
	for (size_t VertexIndex = 0; VertexIndex < vtxSize; VertexIndex += 3)
	{
		const FVertexID VertexID = Description.CreateVertex();
		VertexPositions[VertexID] = FVector(vtx[VertexIndex], vtx[VertexIndex + 2], vtx[VertexIndex + 1]) * 100;
	}
	
	// Create Polygons
	size_t BaseVertexIndex = 0;
	TArray<size_t> BaseUVIndex;
	BaseUVIndex.Init(0, uvSets);
	
	size_t PolygonGroupStartIndex = 0;
	for (size_t PolygonGroupIndex = 0; PolygonGroupIndex < faceRangesSize; ++PolygonGroupIndex)
	{
		const size_t PolygonFaceCount = faceRanges[PolygonGroupIndex];
		
		const FPolygonGroupID PolygonGroupId = Description.CreatePolygonGroup();

		// Create material in game thread
		FGraphEventRef CreateMaterialTask = FFunctionGraphTask::CreateAndDispatchWhenReady([=, &Attributes]()
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_UnrealCallbacks_CreateMaterials);
			const prt::AttributeMap* MaterialAttributes = materials[PolygonGroupIndex];
			UMaterialInstanceDynamic* MaterialInstance = CreateMaterial(Mesh, OpaqueParent, MaterialAttributes);
			const FName MaterialSlot = Mesh->AddMaterial(MaterialInstance);
			Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = MaterialSlot;
		}, TStatId(), nullptr, ENamedThreads::GameThread);

		// Create Geometry
		const auto Normals = Attributes.GetVertexInstanceNormals();
		int PolygonFaces = 0;
		for (size_t FaceIndex = 0; FaceIndex < PolygonFaceCount; ++FaceIndex)
		{
			check(PolygonGroupStartIndex + FaceIndex < faceVertexCountsSize)
			
			const size_t FaceVertexCount = faceVertexCounts[PolygonGroupStartIndex + FaceIndex];
			TArray<FVertexInstanceID> PolygonVertexInstances;

			if (FaceVertexCount >= 3)
			{
				for (size_t FaceVertexIndex = 0; FaceVertexIndex < FaceVertexCount; ++FaceVertexIndex)
				{
					check(BaseVertexIndex + FaceVertexIndex < vertexIndicesSize)
					check(BaseVertexIndex + FaceVertexIndex < normalIndicesSize)
					
					const uint32_t VertexIndex = vertexIndices[BaseVertexIndex + FaceVertexIndex];
					const uint32_t NormalIndex = normalIndices[BaseVertexIndex + FaceVertexIndex] * 3;
					FVertexInstanceID InstanceId = Description.CreateVertexInstance(FVertexID(VertexIndex));
					PolygonVertexInstances.Add(InstanceId);
					
					check(NormalIndex + 2 < nrmSize)
					Normals[InstanceId] = FVector(nrm[NormalIndex], nrm[NormalIndex + 2], nrm[NormalIndex + 1]);

					for (size_t UVSet = 0; UVSet < uvSets; ++UVSet)
					{
						if (uvCounts[UVSet][PolygonGroupStartIndex + FaceIndex] > 0)
						{
							check(uvCounts[UVSet][PolygonGroupStartIndex + FaceIndex] == FaceVertexCount)
							const uint32_t UVIndex = uvIndices[UVSet][BaseUVIndex[UVSet] + FaceVertexIndex] * 2;
							VertexUVs.Set(InstanceId, UVSet, FVector2D(uvs[UVSet][UVIndex], -uvs[UVSet][UVIndex + 1]));
						}
					}
				}

				Description.CreatePolygon(PolygonGroupId, PolygonVertexInstances);
				PolygonFaces++;
				BaseVertexIndex += FaceVertexCount;
				for (size_t UVSet = 0; UVSet < uvSets; ++UVSet)
				{
					BaseUVIndex[UVSet] += uvCounts[UVSet][PolygonGroupStartIndex + FaceIndex];
				}
			}
		}
		
		PolygonGroupStartIndex += PolygonFaces;
		
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(CreateMaterialTask);
	}

	// Build mesh in game thread
	if (BaseVertexIndex > 0) {
		const FGraphEventRef CreateMeshTask = FFunctionGraphTask::CreateAndDispatchWhenReady([this, prototypeId, Mesh, &Description]()
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_UnrealCallbacks_BuildMeshes);
			
			TArray<const FMeshDescription*> MeshDescriptionPtrs;
			MeshDescriptionPtrs.Emplace(&Description);
			Mesh->BuildFromMeshDescriptions(MeshDescriptionPtrs);

			Meshes.Add(prototypeId, Mesh);
		}, TStatId(), nullptr, ENamedThreads::GameThread);
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(CreateMeshTask);
	}
}

void UnrealCallbacks::addInstance(int32_t prototypeId, const double* transform)
{
	const FMatrix TransformationMat(GetRow(transform, 0), GetRow(transform, 1), GetRow(transform, 2), GetRow(transform, 3));
	const int32 SignumDet = FMath::Sign(TransformationMat.Determinant());

	FMatrix MatWithoutScale = TransformationMat.GetMatrixWithoutScale();
	MatWithoutScale = MatWithoutScale * SignumDet;
	MatWithoutScale.M[3][3] = 1;

	const FQuat CERotation = MatWithoutScale.ToQuat();
	const FVector CEScale = TransformationMat.GetScaleVector() * SignumDet;

	// convert from y-up (CE) to z-up (Unreal) (see https://stackoverflow.com/questions/16099979/can-i-switch-x-y-z-in-a-quaternion)
	const FQuat Rotation = FQuat(CERotation.X, CERotation.Z, CERotation.Y, CERotation.W);
	const FVector Scale = FVector(CEScale.X, CEScale.Y, CEScale.Z);

	const FVector Translation = FVector(TransformationMat.M[0][3], TransformationMat.M[2][3], TransformationMat.M[1][3]) * 100;

	const FTransform Transform(Rotation.GetNormalized(), Translation, Scale);

	check(Meshes.Contains(prototypeId))
	Instances.FindOrAdd(Meshes[prototypeId]).Add(Transform);
}

prt::Status UnrealCallbacks::attrBool(size_t isIndex, int32_t shapeID, const wchar_t* key, bool value)
{
	AttributeMapBuilder->setBool(key, value);
	return prt::STATUS_OK;
}

prt::Status UnrealCallbacks::attrFloat(size_t isIndex, int32_t shapeID, const wchar_t* key, double value)
{
	AttributeMapBuilder->setFloat(key, value);
	return prt::STATUS_OK;
}

prt::Status UnrealCallbacks::attrString(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* value)
{
	AttributeMapBuilder->setString(key, value);
	return prt::STATUS_OK;
}

prt::Status UnrealCallbacks::attrBoolArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const bool* values, size_t size)
{
	AttributeMapBuilder->setBoolArray(key, values, size);
	return prt::STATUS_OK;
}

prt::Status UnrealCallbacks::attrFloatArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const double* values, size_t size)
{
	AttributeMapBuilder->setFloatArray(key, values, size);
	return prt::STATUS_OK;
}

prt::Status UnrealCallbacks::attrStringArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* const* values, size_t size)
{
	AttributeMapBuilder->setStringArray(key, values, size);
	return prt::STATUS_OK;
}
