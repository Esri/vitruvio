/* Copyright 2023 Esri
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

// clang-format off
const TMap<Vitruvio::EPrtUvSetType, Vitruvio::EUnrealUvSetType> PRTToUnrealUVSetMap = {
	{Vitruvio::EPrtUvSetType::ColorMap,     Vitruvio::EUnrealUvSetType::ColorMap},
	{Vitruvio::EPrtUvSetType::OpacityMap,   Vitruvio::EUnrealUvSetType::OpacityMap},
	{Vitruvio::EPrtUvSetType::NormalMap,    Vitruvio::EUnrealUvSetType::NormalMap},
	{Vitruvio::EPrtUvSetType::EmissiveMap,  Vitruvio::EUnrealUvSetType::EmissiveMap},
	{Vitruvio::EPrtUvSetType::RoughnessMap, Vitruvio::EUnrealUvSetType::RoughnessMap},
	{Vitruvio::EPrtUvSetType::MetallicMap,  Vitruvio::EUnrealUvSetType::MetallicMap},
	{Vitruvio::EPrtUvSetType::DirtMap,      Vitruvio::EUnrealUvSetType::DirtMap}};

const TMap<Vitruvio::EUnrealUvSetType, FString> UnrealUVSetToMaterialParamStringMap = {
	{Vitruvio::EUnrealUvSetType::DirtMap,      TEXT("HasDirtMapUV")},
	{Vitruvio::EUnrealUvSetType::OpacityMap,   TEXT("HasOpacityMapUV")},
	{Vitruvio::EUnrealUvSetType::NormalMap,    TEXT("HasNormalMapUV")},  
	{Vitruvio::EUnrealUvSetType::EmissiveMap,  TEXT("HasEmissiveMapUV")},
	{Vitruvio::EUnrealUvSetType::RoughnessMap, TEXT("HasRoughnessMapUV")}, 
	{Vitruvio::EUnrealUvSetType::MetallicMap,  TEXT("HasMetallicMapUV")}
};
// clang-format on

TMap<FString, double> CreateAvailableUVSetMaterialParameterMap(uint32_t const* const* UVCounts, size_t UVSets)
{
	TMap<FString, double> AvailableUvSetAttributeMap;

	// Check which uv sets are available and set the corresponding information in the MaterialContainer
	for (size_t PrtUvSet = 0; PrtUvSet < UVSets; ++PrtUvSet)
	{
		const Vitruvio::EUnrealUvSetType* UnrealUVSetPtr = PRTToUnrealUVSetMap.Find(static_cast<Vitruvio::EPrtUvSetType>(PrtUvSet));
		bool bIsValidUnrealUVSet = (UnrealUVSetPtr != nullptr);

		if (bIsValidUnrealUVSet)
		{
			bool bHasUVSet = UVCounts[PrtUvSet] != nullptr;

			const Vitruvio::EUnrealUvSetType UnrealUVSet = *UnrealUVSetPtr;
			if (UnrealUVSet != Vitruvio::EUnrealUvSetType::ColorMap && UnrealUVSet != Vitruvio::EUnrealUvSetType::None)
			{
				const FString UVSetMaterialParamString(UnrealUVSetToMaterialParamStringMap.FindRef(UnrealUVSet));

				if (bHasUVSet)
				{
					AvailableUvSetAttributeMap.Add(UVSetMaterialParamString, 1.0);
				}
			}
		}
	}
	return AvailableUvSetAttributeMap;
}

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

	const auto VertexUVs = Attributes.GetVertexInstanceUVs();
	VertexUVs.SetNumChannels(8);

	// Convert vertices and vertex instances
	const auto VertexPositions = Attributes.GetVertexPositions();
	for (size_t VertexIndex = 0; VertexIndex < vtxSize; VertexIndex += 3)
	{
		const FVertexID VertexID = Description.CreateVertex();
		VertexPositions[VertexID] = FVector3f(vtx[VertexIndex], vtx[VertexIndex + 2], vtx[VertexIndex + 1]) * PRT_TO_UE_SCALE;
	}

	// Create Polygons
	size_t BaseVertexIndex = 0;
	TArray<size_t> BaseUVIndex;
	BaseUVIndex.Init(0, uvSets);

	size_t PolygonGroupStartIndex = 0;
	TArray<Vitruvio::FMaterialAttributeContainer> MeshMaterials;
	TMap<Vitruvio::FMaterialAttributeContainer, FPolygonGroupID> MeshMaterialMap;
	for (size_t PolygonGroupIndex = 0; PolygonGroupIndex < faceRangesSize; ++PolygonGroupIndex)
	{
		const size_t PolygonFaceCount = faceRanges[PolygonGroupIndex];

		Vitruvio::FMaterialAttributeContainer MaterialContainer(materials[PolygonGroupIndex]);
		TMap<FString, double> AvailableUvSetAttributeMap = CreateAvailableUVSetMaterialParameterMap(uvCounts, uvSets);
		for (auto& AvailableUvSetAttribute : AvailableUvSetAttributeMap)
		{
			MaterialContainer.ScalarProperties.Add(AvailableUvSetAttribute);
		}

		FPolygonGroupID PolygonGroupId;
		if (MeshMaterialMap.Contains(MaterialContainer))
		{
			PolygonGroupId = MeshMaterialMap[MaterialContainer];
		}
		else
		{
			MeshMaterials.Add(MaterialContainer);
			PolygonGroupId = Description.CreatePolygonGroup();
			MeshMaterialMap.Add(MaterialContainer, PolygonGroupId);
		}

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
					Normals[InstanceId] = FVector3f(nrm[NormalIndex], nrm[NormalIndex + 2], nrm[NormalIndex + 1]);

					for (size_t PrtUVSet = 0; PrtUVSet < uvSets; ++PrtUVSet)
					{
						const Vitruvio::EUnrealUvSetType* UnrealUVSetPtr = PRTToUnrealUVSetMap.Find(static_cast<Vitruvio::EPrtUvSetType>(PrtUVSet));

						bool bIsValidUnrealUvSet = (UnrealUVSetPtr != nullptr);
						bool bFaceHasUvs = uvCounts[PrtUVSet] != nullptr && uvCounts[PrtUVSet][PolygonGroupStartIndex + FaceIndex] > 0;

						if (bIsValidUnrealUvSet && bFaceHasUvs)
						{
							check(uvCounts[PrtUVSet][PolygonGroupStartIndex + FaceIndex] == FaceVertexCount);
							const uint32_t UVIndex = uvIndices[PrtUVSet][BaseUVIndex[PrtUVSet] + FaceVertexIndex] * 2;
							FVector2f UVCoords = FVector2f(uvs[PrtUVSet][UVIndex], -uvs[PrtUVSet][UVIndex + 1]);
							VertexUVs.Set(InstanceId, static_cast<int32>(*UnrealUVSetPtr), UVCoords);
						}
					}
				}

				Description.CreatePolygon(PolygonGroupId, PolygonVertexInstances);
				PolygonFaces++;
				BaseVertexIndex += FaceVertexCount;
				for (size_t PrtUVSet = 0; PrtUVSet < uvSets; ++PrtUVSet)
				{
					if (uvCounts[PrtUVSet] != nullptr)
					{
						BaseUVIndex[PrtUVSet] += uvCounts[PrtUVSet][PolygonGroupStartIndex + FaceIndex];
					}
				}
			}
		}

		PolygonGroupStartIndex += PolygonFaces;
	}

	if (BaseVertexIndex > 0)
	{
		Description.TriangulateMesh();

		bool bHasInvalidNormals;
		bool bHasInvalidTangents;
		FStaticMeshOperations::AreNormalsAndTangentsValid(Description, bHasInvalidNormals, bHasInvalidTangents);

		// If normals are invalid, compute normals and tangents at polygon level then vertex level
		if (bHasInvalidNormals)
		{
			FStaticMeshOperations::ComputeTriangleTangentsAndNormals(Description, THRESH_POINTS_ARE_SAME);

			const EComputeNTBsFlags ComputeFlags = EComputeNTBsFlags::Normals | EComputeNTBsFlags::Tangents | EComputeNTBsFlags::UseMikkTSpace;
			FStaticMeshOperations::ComputeTangentsAndNormals(Description, ComputeFlags);
		}
		else if (bHasInvalidTangents)
		{
			FStaticMeshOperations::ComputeMikktTangents(Description, true);
		}

		TSharedPtr<FVitruvioMesh> Mesh = MakeShared<FVitruvioMesh>(UriString, Description, MeshMaterials);

		if (!UriString.IsEmpty())
		{
			Mesh = VitruvioModule::Get().GetMeshCache().InsertOrGet(UriString, Mesh);
		}

		Meshes.Add(prototypeId, Mesh);
		Names.Add(prototypeId, NameString);
	}
}

TMap<FString, FReport> ExtractReports(const prt::AttributeMap* reports)
{
	TMap<FString, FReport> ReportMap;
	size_t KeyCount = 0;
	auto Keys = reports->getKeys(&KeyCount);
	ReportMap.Reserve(KeyCount);

	for (size_t i = 0; i < KeyCount; ++i)
	{
		auto key = Keys[i];

		FReport Report;
		Report.Name = key;
		switch (reports->getType(key))
		{
		case prt::AttributeMap::PrimitiveType::PT_BOOL:
			Report.Type = EReportPrimitiveType::Bool;
			Report.Value = reports->getBool(key) ? TEXT("true") : TEXT("false");
			break;
		case prt::AttributeMap::PrimitiveType::PT_STRING:
			Report.Type = EReportPrimitiveType::String;
			Report.Value = reports->getString(key);
			break;
		case prt::AttributeMap::PrimitiveType::PT_FLOAT:
			Report.Type = EReportPrimitiveType::Float;
			Report.Value = FString::SanitizeFloat(reports->getFloat(key));
			break;
		case prt::AttributeMap::PrimitiveType::PT_INT:
			Report.Type = EReportPrimitiveType::Int;
			Report.Value = FString::FromInt(reports->getInt(key));
			break;
		default:
			UE_LOG(LogUnrealCallbacks, Error, TEXT("Type of report not supported."));
			break;
		}

		ReportMap.Add(Report.Name, Report);
	}
	return ReportMap;
}

void UnrealCallbacks::addReport(const prt::AttributeMap* reports)
{
	if (!reports)
	{
		UE_LOG(LogUnrealCallbacks, Warning, TEXT("Trying to add empty report, ignoring."));
		return;
	}

	Reports = ExtractReports(reports);
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
