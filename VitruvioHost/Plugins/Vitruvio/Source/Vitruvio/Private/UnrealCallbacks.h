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

#include "PRTTypes.h"

#include "Codec/Encoder/IUnrealCallbacks.h"
#include "Report.h"
#include "VitruvioTypes.h"

#include "Core.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "Modules/ModuleManager.h"
#include "VitruvioMesh.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUnrealCallbacks, Log, All);

class UnrealCallbacks final : public IUnrealCallbacks
{
	AttributeMapBuilderUPtr& AttributeMapBuilder;

	Vitruvio::FInstanceMap Instances;
	TMap<int32, TSharedPtr<FVitruvioMesh>> Meshes;
	TMap<FString, FReport> Reports;
	TMap<int32, FString> Names;

public:
	virtual ~UnrealCallbacks() override = default;
	UnrealCallbacks(AttributeMapBuilderUPtr& AttributeMapBuilder) : AttributeMapBuilder(AttributeMapBuilder) {}

	static const int32 NO_PROTOTYPE_INDEX = -1;

	const Vitruvio::FInstanceMap& GetInstances() const
	{
		return Instances;
	}

	TSharedPtr<FVitruvioMesh> GetMeshById(int32 PrototypeId) const
	{
		return Meshes[PrototypeId];
	}

	const TMap<int32, TSharedPtr<FVitruvioMesh>>& GetMeshes() const
	{
		return Meshes;
	}

	const TMap<FString, FReport>& GetReports() const
	{
		return Reports;
	}

	const TMap<int32, FString>& GetNames() const
	{
		return Names;
	}

	/**
	 * @param name initial shape name, optionally used to create primitive groups on output
	 * @param prototypeId the id of the prototype or -1 of not cached
	 * @param uri
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
	void addMesh(const wchar_t* name,
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

	prt::Status generateError(size_t /*isIndex*/, prt::Status /*status*/, const wchar_t* message) override
	{
		UE_LOG(LogUnrealCallbacks, Error, TEXT("GENERATE ERROR: %s"), message)
		return prt::STATUS_OK;
	}
	prt::Status assetError(size_t /*isIndex*/, prt::CGAErrorLevel /*level*/, const wchar_t* /*key*/, const wchar_t* /*uri*/,
						   const wchar_t* message) override
	{
		UE_LOG(LogUnrealCallbacks, Error, TEXT("ASSET ERROR: %s"), message)
		return prt::STATUS_OK;
	}
	prt::Status cgaError(size_t /*isIndex*/, int32_t /*shapeID*/, prt::CGAErrorLevel /*level*/, int32_t /*methodId*/, int32_t /*pc*/,
						 const wchar_t* message) override
	{
		UE_LOG(LogUnrealCallbacks, Error, TEXT("CGA ERROR: %s"), message)
		return prt::STATUS_OK;
	}
	prt::Status cgaPrint(size_t /*isIndex*/, int32_t /*shapeID*/, const wchar_t* txt) override
	{
		UE_LOG(LogUnrealCallbacks, Display, TEXT("CGA Print: %s"), txt)
		return prt::STATUS_OK;
	}

	prt::Status cgaReportBool(size_t isIndex, int32_t shapeID, const wchar_t* key, bool value) override
	{
		return prt::STATUS_OK;
	}
	prt::Status cgaReportFloat(size_t isIndex, int32_t shapeID, const wchar_t* key, double value) override
	{
		return prt::STATUS_OK;
	}
	prt::Status cgaReportString(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* value) override
	{
		return prt::STATUS_OK;
	}

	prt::Status attrBool(size_t isIndex, int32_t shapeID, const wchar_t* key, bool value) override;
	prt::Status attrFloat(size_t isIndex, int32_t shapeID, const wchar_t* key, double value) override;
	prt::Status attrString(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* value) override;

	prt::Status attrBoolArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const bool* values, size_t size, size_t nRows) override;
	prt::Status attrFloatArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const double* values, size_t size, size_t nRows) override;
	prt::Status attrStringArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* const* values, size_t size,
								size_t nRows) override;
};
