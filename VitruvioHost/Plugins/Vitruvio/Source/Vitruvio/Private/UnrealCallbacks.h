/* Copyright 2022 Esri
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

#include "PRTTypes.h"

#include "Codec/Encoder/IUnrealCallbacks.h"
#include "Report.h"
#include "VitruvioTypes.h"

#include "Core.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Modules/ModuleManager.h"
#include "VitruvioMesh.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUnrealCallbacks, Log, All);

struct FModelDescription
{
	FMeshDescription MeshDescription;
	size_t VertexIndexOffset = 0;
	TArray<Vitruvio::FMaterialAttributeContainer> Materials;
	TMap<Vitruvio::FMaterialAttributeContainer, FPolygonGroupID> MaterialToPolygonMap;
};

class UnrealCallbacks final : public IUnrealCallbacks
{
	TArray<AttributeMapBuilderUPtr>& AttributeMapBuilders;

	Vitruvio::FInstanceMap Instances;
	TMap<int32, TSharedPtr<FVitruvioMesh>> InstanceMeshes;
	TMap<int32, FString> InstanceNames;

	FModelDescription ModelDescription;
	TSharedPtr<FVitruvioMesh> GeneratedModel;
	TMap<FString, FReport> Reports;
	
public:
	virtual ~UnrealCallbacks() override = default;
	UnrealCallbacks(TArray<AttributeMapBuilderUPtr>& AttributeMapBuilders) : AttributeMapBuilders(AttributeMapBuilders) {}

	static constexpr int32 NoPrototypeIndex = -1;

	const Vitruvio::FInstanceMap& GetInstances() const
	{
		return Instances;
	}

	const TMap<int32, TSharedPtr<FVitruvioMesh>>& GetInstanceMeshes() const
	{
		return InstanceMeshes;
	}

	const TSharedPtr<FVitruvioMesh>& GetGeneratedModel() const
	{
		return GeneratedModel;
	}

	const TMap<FString, FReport>& GetReports() const
	{
		return Reports;
	}

	const TMap<int32, FString>& GetInstanceNames() const
	{
		return InstanceNames;
	}

	/**
	 * @param name either the name of the inserted asset or the shape name
	 * @param identifier unique identifier of this mesh if originates from an inserted asset or empty otherwise
	 * @param prototypeId the id of the prototype or -1 of not cached
	 * @param uri the uri of the inserted asset or empty otherwise
	 * @param vtx vertex coordinate array
	 * @param vtxSize of vertex coordinate array
	 * @param nrm vertex normal array
	 * @param nrmSize length of vertex normal array
	 * @param faceVertexCounts vertex counts per face
	 * @param faceVertexCountsSize number of faces (= size of faceCounts)
	 * @param vertexIndices vertex attribute index array (grouped by counts)
	 * @param vertexIndicesSize vertex attribute index array
	 * @param uvs array of texture coordinate arrays (same indexing as vertices per uv set)
	 * @param uvsSizes lengths of uv arrays per uv set
	 * @param faceRanges ranges for materials and reports
	 * @param materials contains faceRangesSize-1 attribute maps (all materials must have an identical set of keys and
	 * types)
	 */
	// clang-format off
	virtual void addMesh(const wchar_t* name, const wchar_t* identifier,
	                     int32_t prototypeId, const wchar_t* uri,
	                     const double* vtx, size_t vtxSize,
	                     const double* nrm, size_t nrmSize,
	                     const uint32_t* faceVertexCounts, size_t faceVertexCountsSize,
	                     const uint32_t* vertexIndices, size_t vertexIndicesSize,
	                     const uint32_t* normalIndices, size_t normalIndicesSize,

	                     double const* const* uvs, size_t const* uvsSizes,
	                     uint32_t const* const* uvCounts, size_t const* uvCountsSizes,
	                     uint32_t const* const* uvIndices, size_t const* uvIndicesSizes,
	                     size_t uvSets,

                         const uint32_t* faceRanges, size_t faceRangesSize,
	                     const prt::AttributeMap** materials
	) override;
	// clang-format on

	/**
	 * Add a new instance with a given id, transform and optional set of overriding attributes for this instance
	 *
	 * @param prototypeId the id of the prorotype. An @ref addMesh call with the specified prorotypeId will be called before
	 *                    the call to addInstance
	 * @param transform the transformation matrix of this instance
	 * @param instanceMaterial override materials for this instance
	 * @param numInstanceMaterials number of instance material overrides. Is either 0 or is equal to the number
	 *                             of materials of the original mesh (by prototypeId)
	 */
	virtual void addInstance(int32_t prototypeId, const double* transform, const prt::AttributeMap** instanceMaterial,
							 size_t numInstanceMaterials) override;

	/**
	 * Add a new report
	 *
	 * @param reports an attribute map that stores all report attributes
	 */
	virtual void addReport(const prt::AttributeMap* reports) override;

	virtual void init() override;
	
	virtual void finish() override;

	virtual prt::Status generateError(size_t /*isIndex*/, prt::Status /*status*/, const wchar_t* message) override
	{
		UE_LOG(LogUnrealCallbacks, Error, TEXT("GENERATE ERROR: %s"), message)
		return prt::STATUS_OK;
	}
	virtual prt::Status assetError(size_t /*isIndex*/, prt::CGAErrorLevel /*level*/, const wchar_t* /*key*/, const wchar_t* /*uri*/,
						   const wchar_t* message) override
	{
		UE_LOG(LogUnrealCallbacks, Error, TEXT("ASSET ERROR: %s"), message)
		return prt::STATUS_OK;
	}
	virtual prt::Status cgaError(size_t /*isIndex*/, int32_t /*shapeID*/, prt::CGAErrorLevel /*level*/, int32_t /*methodId*/, int32_t /*pc*/,
						 const wchar_t* message) override
	{
		UE_LOG(LogUnrealCallbacks, Error, TEXT("CGA ERROR: %s"), message)
		return prt::STATUS_OK;
	}
	virtual prt::Status cgaPrint(size_t /*isIndex*/, int32_t /*shapeID*/, const wchar_t* txt) override
	{
		UE_LOG(LogUnrealCallbacks, Display, TEXT("CGA Print: %s"), txt)
		return prt::STATUS_OK;
	}

	virtual prt::Status cgaReportBool(size_t isIndex, int32_t shapeID, const wchar_t* key, bool value) override
	{
		return prt::STATUS_OK;
	}
	virtual prt::Status cgaReportFloat(size_t isIndex, int32_t shapeID, const wchar_t* key, double value) override
	{
		return prt::STATUS_OK;
	}
	virtual prt::Status cgaReportString(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* value) override
	{
		return prt::STATUS_OK;
	}

	virtual prt::Status attrBool(size_t isIndex, int32_t shapeID, const wchar_t* key, bool value) override;
	virtual prt::Status attrFloat(size_t isIndex, int32_t shapeID, const wchar_t* key, double value) override;
	virtual prt::Status attrString(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* value) override;

	virtual prt::Status attrBoolArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const bool* values, size_t size, size_t nRows) override;
	virtual prt::Status attrFloatArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const double* values, size_t size, size_t nRows) override;
	virtual prt::Status attrStringArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* const* values, size_t size,
								size_t nRows) override;

};