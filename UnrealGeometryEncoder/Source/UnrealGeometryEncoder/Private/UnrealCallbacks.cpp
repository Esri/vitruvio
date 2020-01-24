#pragma once

#include "UnrealCallbacks.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshDescription.h"
#include "StaticMeshOperations.h"

DEFINE_LOG_CATEGORY(LogUnrealCallbacks);

void UnrealCallbacks::addMesh(const wchar_t* name, const double* vtx, size_t vtxSize, const double* nrm, size_t nrmSize, const uint32_t* faceVertexCounts,
							  size_t faceVertexCountsSize, const uint32_t* vertexIndices, size_t vertexIndicesSize, const uint32_t* normalIndices, size_t normalIndicesSize,

							  double const* const* uvs, size_t const* uvsSizes, uint32_t const* const* uvCounts, size_t const* uvCountsSizes, uint32_t const* const* uvIndices,
							  size_t const* uvIndicesSizes, size_t uvSets)
{
	UStaticMesh* Mesh = NewObject<UStaticMesh>();

	UMaterial* Material = NewObject<UMaterial>();
	UMaterialExpressionConstant* ConstantColor = NewObject<UMaterialExpressionConstant>(Material);
	ConstantColor->R = 0.2f;
	Material->Expressions.Add(ConstantColor);
	Material->BaseColor.Expression = ConstantColor;

	Material->TwoSided = true;

	FName MaterialSlot = Mesh->AddMaterial(Material);

	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();

	// Convert vertices and vertex instances
	const auto VertexPositions = Attributes.GetVertexPositions();
	for (size_t VertexIndex = 0; VertexIndex < vtxSize; VertexIndex += 3)
	{
		const FVertexID VertexID = Description.CreateVertex();
		VertexPositions[VertexID] = FVector(vtx[VertexIndex], vtx[VertexIndex + 1], -vtx[VertexIndex + 2]);
	}

	// Create Polygons
	size_t BaseVertexIndex = 0;
	const FPolygonGroupID PolygonGroupId = Description.CreatePolygonGroup();
	Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = MaterialSlot;
	const auto Normals = Attributes.GetVertexInstanceNormals();
	for (size_t FaceIndex = 0; FaceIndex < faceVertexCountsSize; ++FaceIndex)
	{
		const size_t FaceVertexCount = faceVertexCounts[FaceIndex];

		TArray<FVertexInstanceID> PolygonVertexInstances;
		for (size_t FaceVertexIndex = 0; FaceVertexIndex < FaceVertexCount; ++FaceVertexIndex)
		{
			const uint32_t VertexIndex = vertexIndices[BaseVertexIndex + FaceVertexIndex];
			const uint32_t NormalIndex = normalIndices[BaseVertexIndex + FaceVertexIndex];
			FVertexInstanceID InstanceId = Description.CreateVertexInstance(FVertexID(VertexIndex));
			PolygonVertexInstances.Add(InstanceId);
			Normals[InstanceId] = FVector(nrm[NormalIndex * 3], nrm[(NormalIndex * 3) + 1], -nrm[(NormalIndex * 3) + 2]);
		}

		Description.CreatePolygon(PolygonGroupId, PolygonVertexInstances);
		BaseVertexIndex += FaceVertexCount;
	}

	// Create and add mesh
	TArray<const FMeshDescription*> MeshDescriptionPtrs;
	MeshDescriptionPtrs.Emplace(&Description);

	Mesh->BuildFromMeshDescriptions(MeshDescriptionPtrs);
	Meshes.Add(Mesh);
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
