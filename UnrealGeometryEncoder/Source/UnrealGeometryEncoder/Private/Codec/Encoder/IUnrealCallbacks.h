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
	 * @param name initial shape (primitive group) name, optionally used to create primitive groups on output
	 * @param vtx vertex coordinate array
	 * @param vtxSize of vertex coordinate array
	 * @param nrm vertex normal array
	 * @param nrmSize length of vertex normal array
	 * @param faceVertexCounts vertex counts per face
	 * @param faceVertexCountsSize number of faces (= size of faceCounts)
	 * @param indices vertex attribute index array (grouped by counts)
	 * @param indicesSize vertex attribute index array
	 * @param uvs array of texture coordinate arrays (same indexing as vertices per uv set)
	 * @param uvsSizes lengths of uv arrays per uv set
	 */
	// clang-format off
	virtual void addMesh(const wchar_t* name,
	                     const double* vtx, size_t vtxSize,
	                     const double* nrm, size_t nrmSize,
	                     const uint32_t* faceVertexCounts, size_t faceVertexCountsSize,
	                     const uint32_t* vertexIndices, size_t vertexIndicesSize,
	                     const uint32_t* normalIndices, size_t normalIndicesSize,

	                     double const* const* uvs, size_t const* uvsSizes,
	                     uint32_t const* const* uvCounts, size_t const* uvCountsSizes,
	                     uint32_t const* const* uvIndices, size_t const* uvIndicesSizes,
	                     size_t uvSets

	) = 0;
	// clang-format on
};