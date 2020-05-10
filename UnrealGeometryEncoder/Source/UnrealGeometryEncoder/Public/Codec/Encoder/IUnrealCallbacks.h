// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "prt/Callbacks.h"

constexpr const wchar_t* ENCODER_ID_UnrealGeometry = L"UnrealGeometryEncoder";
constexpr const wchar_t* EO_EMIT_ATTRIBUTES = L"emitAttributes";
constexpr const wchar_t* EO_EMIT_MATERIALS = L"emitMaterials";
constexpr const wchar_t* EO_EMIT_REPORTS = L"emitReports";

class IUnrealCallbacks : public prt::Callbacks
{
public:
	~IUnrealCallbacks() override = default;

	/**
	 * @param name initial shape name, optionally used to create primitive groups on output
	 * @param prototypeId the id of the prototype or -1 of not cached
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
	virtual void addMesh(const wchar_t* name,
	                     int32_t prototypeId,
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
	) = 0;
	// clang-format on

	/**
	 * @param prototypeId
	 * @param transform
	 */
	virtual void addInstance(int32_t prototypeId, const double* transform) = 0;
};
