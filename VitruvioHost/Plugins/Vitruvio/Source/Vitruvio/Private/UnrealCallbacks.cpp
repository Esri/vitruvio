/* Copyright 2021 Esri
 *
 * Licensed under the Apache License Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "UnrealCallbacks.h"

#include "Util/MaterialConversion.h"

#include "Engine/StaticMesh.h"
#include "IImageWrapper.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshDescription.h"
#include "StaticMeshOperations.h"
#include "Util/AsyncHelpers.h"
#include "VitruvioModule.h"
#include "prtx/Mesh.h"

DEFINE_LOG_CATEGORY(LogUnrealCallbacks);

namespace
{

FPlane GetColumn(const double* Mat, int32 Index)
{
	return FPlane(Mat[Index * 4 + 0], Mat[Index * 4 + 1], Mat[Index * 4 + 2], Mat[Index * 4 + 3]);
}

FQuat Conjugate(const FQuat& In)
{
	FQuat Res = In;
	Res.X = -In.X;
	Res.Y = -In.Y;
	Res.Z = -In.Z;
	return Res;
}

// Standard conversion from meters (PRT) to centimeters (UE4)
constexpr float PRT_TO_UE_SCALE = 100.0f;

// Note that we use the same tolerance (1e-25f) as in PRT to avoid numerical issues when converting planar geometry
constexpr float PRT_DIVISOR_LIMIT = 1e-25f;

} // namespace

void UnrealCallbacks::addMesh(const wchar_t* name, int32_t prototypeId, const wchar_t* uri, const double* vtx, size_t vtxSize, const double* nrm,
							  size_t nrmSize, const uint32_t* faceVertexCounts, size_t faceVertexCountsSize, const uint32_t* vertexIndices,
							  size_t vertexIndicesSize, const uint32_t* normalIndices, size_t normalIndicesSize,

							  double const* const* uvs, size_t const* uvsSizes, uint32_t const* const* uvCounts, size_t const* uvCountsSizes,
							  uint32_t const* const* uvIndices, size_t const* uvIndicesSizes, size_t uvSets,

							  const uint32_t* faceRanges, size_t faceRangesSize, const prt::AttributeMap** materials)
{

	const FString UriString(uri);
	const FString NameString(name);
	
	if (!UriString.IsEmpty())
	{
		TSharedPtr<FVitruvioMesh> Mesh = VitruvioModule::Get().GetMeshCache().Get(UriString);
		if (Mesh)
		{
			Meshes.Add(prototypeId, Mesh);
			Names.Add(prototypeId, NameString);
			return;
		}
	}

	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();

	if (uvSets > 8)
	{
		UE_LOG(LogUnrealCallbacks, Error, TEXT("Mesh %s uses %llu UV sets but only 8 are allowed. Clamping UV sets to 8."), name, uvSets);
		uvSets = 8;
	}

	// Need at least 1 uv set (can be empty) otherwise will crash when building the mesh
	const auto VertexUVs = Attributes.GetVertexInstanceUVs();
	VertexUVs.SetNumIndices(FMath::Max(static_cast<size_t>(1), uvSets));

	// Convert vertices and vertex instances
	const auto VertexPositions = Attributes.GetVertexPositions();
	for (size_t VertexIndex = 0; VertexIndex < vtxSize; VertexIndex += 3)
	{
		const FVertexID VertexID = Description.CreateVertex();
		VertexPositions[VertexID] = FVector(vtx[VertexIndex], vtx[VertexIndex + 2], vtx[VertexIndex + 1]) * PRT_TO_UE_SCALE;
	}

	// Create Polygons
	size_t BaseVertexIndex = 0;
	TArray<size_t> BaseUVIndex;
	BaseUVIndex.Init(0, uvSets);

	size_t PolygonGroupStartIndex = 0;
	TArray<Vitruvio::FMaterialAttributeContainer> MeshMaterials;
	for (size_t PolygonGroupIndex = 0; PolygonGroupIndex < faceRangesSize; ++PolygonGroupIndex)
	{
		const size_t PolygonFaceCount = faceRanges[PolygonGroupIndex];

		const FPolygonGroupID PolygonGroupId = Description.CreatePolygonGroup();

		Vitruvio::FMaterialAttributeContainer MaterialContainer(materials[PolygonGroupIndex]);
		const FName MaterialSlot = FName(MaterialContainer.Name);
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = MaterialSlot;
		MeshMaterials.Add(MaterialContainer);

		// Create Geometry
		const auto Normals = Attributes.GetVertexInstanceNormals();
		int PolygonFaces = 0;
		for (size_t FaceIndex = 0; FaceIndex < PolygonFaceCount; ++FaceIndex)
		{
			check(PolygonGroupStartIndex + FaceIndex < faceVertexCountsSize);

			const size_t FaceVertexCount = faceVertexCounts[PolygonGroupStartIndex + FaceIndex];
			TArray<FVertexInstanceID> PolygonVertexInstances;

			if (FaceVertexCount >= 3)
			{
				for (size_t FaceVertexIndex = 0; FaceVertexIndex < FaceVertexCount; ++FaceVertexIndex)
				{
					check(BaseVertexIndex + FaceVertexIndex < vertexIndicesSize);
					check(BaseVertexIndex + FaceVertexIndex < normalIndicesSize);

					const uint32_t VertexIndex = vertexIndices[BaseVertexIndex + FaceVertexIndex];
					const uint32_t NormalIndex = normalIndices[BaseVertexIndex + FaceVertexIndex] * 3;
					FVertexInstanceID InstanceId = Description.CreateVertexInstance(FVertexID(VertexIndex));
					PolygonVertexInstances.Add(InstanceId);

					check(NormalIndex + 2 < nrmSize);
					Normals[InstanceId] = FVector(nrm[NormalIndex], nrm[NormalIndex + 2], nrm[NormalIndex + 1]);

					for (size_t UVSet = 0; UVSet < uvSets; ++UVSet)
					{
						if (uvCounts[UVSet] != nullptr && uvCounts[UVSet][PolygonGroupStartIndex + FaceIndex] > 0)
						{
							check(uvCounts[UVSet][PolygonGroupStartIndex + FaceIndex] == FaceVertexCount);
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
					if (uvCounts[UVSet] != nullptr)
					{
						BaseUVIndex[UVSet] += uvCounts[UVSet][PolygonGroupStartIndex + FaceIndex];
					}
				}
			}
		}

		PolygonGroupStartIndex += PolygonFaces;
	}

	if (BaseVertexIndex > 0)
	{
		TSharedPtr<FVitruvioMesh> Mesh = MakeShared<FVitruvioMesh>(UriString, Description, MeshMaterials);

		if (!UriString.IsEmpty())
		{
			Mesh = VitruvioModule::Get().GetMeshCache().InsertOrGet(UriString, Mesh);
		}
		
		Meshes.Add(prototypeId, Mesh);
		Names.Add(prototypeId, NameString);
	}
}

void UnrealCallbacks::addInstance(int32_t prototypeId, const double* transform, const prt::AttributeMap** instanceMaterials,
								  size_t numInstanceMaterials)
{
	const FMatrix TransformationMat(GetColumn(transform, 0), GetColumn(transform, 1), GetColumn(transform, 2), GetColumn(transform, 3));
	const int32 SignumDet = FMath::Sign(TransformationMat.Determinant());

	// Create proper rotation matrix (remove scaling and translation and det == 1)
	FMatrix RotationMat = TransformationMat.GetMatrixWithoutScale(PRT_DIVISOR_LIMIT).RemoveTranslation();
	RotationMat = RotationMat * SignumDet;
	RotationMat.M[3][3] = 1;

	const FQuat Rotation =
		Conjugate(RotationMat.ToQuat()); // Conjugate because we want the quaternion to describe a transformation to basis vectors of RotationMat
	const FVector Scale = TransformationMat.GetScaleVector() * SignumDet;
	const FVector Translation = TransformationMat.GetOrigin();

	// Convert from right-handed y-up (CE) to left-handed z-up (Unreal) (see
	// https://stackoverflow.com/questions/16099979/can-i-switch-x-y-z-in-a-quaternion)
	const FQuat CERotation = FQuat(Rotation.X, Rotation.Z, Rotation.Y, Rotation.W);
	const FVector CEScale = FVector(Scale.X, Scale.Z, Scale.Y);
	const FVector CETranslation = FVector(Translation.X, Translation.Z, Translation.Y) * PRT_TO_UE_SCALE;

	if (!Meshes.Contains(prototypeId))
	{
		UE_LOG(LogUnrealCallbacks, Warning, TEXT("No mesh found for prototypeId %d"), prototypeId);
		return;
	}

	const FTransform Transform(CERotation.GetNormalized(), CETranslation, CEScale);

	TArray<Vitruvio::FMaterialAttributeContainer> MaterialOverrides;
	if (instanceMaterials)
	{
		for (size_t MatIndex = 0; MatIndex < numInstanceMaterials; ++MatIndex)
		{
			Vitruvio::FMaterialAttributeContainer MaterialContainer(instanceMaterials[MatIndex]);
			MaterialOverrides.Add(MaterialContainer);
		}
	}

	Instances.FindOrAdd({prototypeId, MaterialOverrides}).Add(Transform);
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

prt::Status UnrealCallbacks::attrBoolArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const bool* values, size_t size, size_t nRows)
{
	AttributeMapBuilder->setBoolArray(key, values, size);
	return prt::STATUS_OK;
}

prt::Status UnrealCallbacks::attrFloatArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const double* values, size_t size, size_t nRows)
{
	AttributeMapBuilder->setFloatArray(key, values, size);
	return prt::STATUS_OK;
}

prt::Status UnrealCallbacks::attrStringArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* const* values, size_t size,
											 size_t nRows)
{
	AttributeMapBuilder->setStringArray(key, values, size);
	return prt::STATUS_OK;
}
