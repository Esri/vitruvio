#pragma once

#include "Codec/Encoder/IUnrealCallbacks.h"
#include "Core.h"
#include "Engine/StaticMesh.h"
#include "Util/UnrealPRTUtils.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUnrealCallbacks, Log, All);

class UnrealCallbacks : public IUnrealCallbacks
{
	AttributeMapBuilderUPtr& AttributeMapBuilder;
	UStaticMesh* Mesh;

public:
	~UnrealCallbacks() override = default;
	UnrealCallbacks(AttributeMapBuilderUPtr& AttributeMapBuilder) : AttributeMapBuilder(AttributeMapBuilder)
	{
	}

	/**
	 * @param name initial shape (primitive group) name, optionally used to create primitive groups on output
	 * @param vtx vertex coordinate array
	 * @param vtxSize of vertex coordinate array
	 * @param nrm vertex normal array
	 * @param nrmSize length of vertex normal array
	 * @param faceVertexCounts vertex counts per face
	 * @param faceVertexCountsSize number of faces (= size of faceCounts)
	 * @param vertexIndices vertex attribute index array (grouped by counts)
	 * @param vertexIndicesSize vertex attribute index array
	 * @param normalIndices vertex attribute index array (grouped by counts)
	 * @param normalIndicesSize vertex attribute index array
	 * @param uvs array of texture coordinate arrays (same indexing as vertices per uv set)
	 * @param uvsSizes lengths of uv arrays per uv set
	 * @param uvSetsCount number of uv sets
	 * @param faceRanges ranges for materials and reports
	 * @param materials contains faceRangesSize-1 attribute maps (all materials must have an identical set of keys and
	 * types)
	 * @param reports contains faceRangesSize-1 attribute maps
	 * @param shapeIDs shape ids per face, contains faceRangesSize-1 values
	 */
	// clang-format off
	void setMesh(const wchar_t* name,
		const double* vtx, size_t vtxSize,
		const double* nrm, size_t nrmSize,
		const uint32_t* faceVertexCounts, size_t faceVertexCountsSize,
		const uint32_t* vertexIndices, size_t vertexIndicesSize,
		const uint32_t* normalIndices, size_t normalIndicesSize,

		double const* const* uvs, size_t const* uvsSizes,
		uint32_t const* const* uvCounts, size_t const* uvCountsSizes,
		uint32_t const* const* uvIndices, size_t const* uvIndicesSizes,
		size_t uvSets
	) override;
	// clang-format on

	UStaticMesh* getMesh() const
	{
		return Mesh;
	}

	prt::Status generateError(size_t /*isIndex*/, prt::Status /*status*/, const wchar_t* message) override
	{
		UE_LOG(LogUnrealCallbacks, Error, TEXT("GENERATE ERROR: %s"), message)
		return prt::STATUS_OK;
	}
	prt::Status assetError(size_t /*isIndex*/, prt::CGAErrorLevel /*level*/, const wchar_t* /*key*/, const wchar_t* /*uri*/, const wchar_t* message) override
	{
		UE_LOG(LogUnrealCallbacks, Error, TEXT("ASSET ERROR: %s"), message)
		return prt::STATUS_OK;
	}
	prt::Status cgaError(size_t /*isIndex*/, int32_t /*shapeID*/, prt::CGAErrorLevel /*level*/, int32_t /*methodId*/, int32_t /*pc*/, const wchar_t* message) override
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
	prt::Status attrBoolArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const bool* ptr, size_t size) override;
	prt::Status attrFloatArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const double* ptr, size_t size) override;
	prt::Status attrStringArray(size_t isIndex, int32_t shapeID, const wchar_t* key, const wchar_t* const* ptr, size_t size) override;
};
