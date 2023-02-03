/* Copyright 2020 Esri
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

#include "UnrealGeometryEncoder.h"

#include "Codec/Encoder/IUnrealCallbacks.h"

#pragma warning(push)
#pragma warning(disable : 4263 4264)
#include "prtx/Attributable.h"
#include "prtx/Exception.h"
#include "prtx/ExtensionManager.h"
#include "prtx/GenerateContext.h"
#include "prtx/Geometry.h"
#include "prtx/Log.h"
#include "prtx/Material.h"
#include "prtx/Mesh.h"
#include "prtx/ReportsCollector.h"
#include "prtx/Shape.h"
#include "prtx/ShapeIterator.h"
#pragma warning(pop)

#include <algorithm>
#include <cassert>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <vector>

namespace
{
constexpr bool DBG = false;

constexpr const wchar_t* UNREAL_GEOMETRY_ENCODER_NAME = L"Unreal Encoder";
constexpr const wchar_t* UNREAL_GEOMETRY_ENCODER_DESCRIPTION = L"Encodes geometry into Unreal geometry.";

constexpr const wchar_t* EO_EMIT_ATTRIBUTES = L"emitAttributes";
constexpr const wchar_t* EO_EMIT_MATERIALS = L"emitMaterials";
constexpr const wchar_t* EO_EMIT_REPORTS = L"emitReports";

const prtx::DoubleVector EMPTY_UVS;
const prtx::IndexVector EMPTY_IDX;

struct SerializedGeometry
{
	prtx::DoubleVector coords;
	prtx::DoubleVector normals;
	std::vector<uint32_t> faceVertexCounts;
	std::vector<uint32_t> vertexIndices;
	std::vector<uint32_t> normalIndices;

	std::vector<prtx::DoubleVector> uvs;
	std::vector<prtx::IndexVector> uvCounts;
	std::vector<prtx::IndexVector> uvIndices;

	SerializedGeometry(uint32_t numCounts, uint32_t numIndices, uint32_t uvSets) : uvs(uvSets), uvCounts(uvSets), uvIndices(uvSets)
	{
		faceVertexCounts.reserve(numCounts);
		vertexIndices.reserve(numIndices);
		normalIndices.reserve(numIndices);
	}
};

using AttributeMapNOPtrVector = std::vector<const prt::AttributeMap*>;

struct AttributeMapNOPtrVectorOwner
{
	AttributeMapNOPtrVector v;

	~AttributeMapNOPtrVectorOwner()
	{
		for (const auto& m : v)
		{
			if (m != nullptr)
				m->destroy();
		}
	}
};

struct TextureUVMapping
{
	std::wstring key;
	uint8_t index;
	int8_t uvSet;
};

const std::vector<TextureUVMapping> TEXTURE_UV_MAPPINGS = {
	// shader key   | idx | uv set  | CGA key
	{L"diffuseMap", 0, 0},	 // colormap
	{L"bumpMap", 0, 1},		 // bumpmap
	{L"diffuseMap", 1, 2},	 // dirtmap
	{L"specularMap", 0, 3},	 // specularmap
	{L"opacityMap", 0, 4},	 // opacitymap
	{L"normalMap", 0, 5},	 // normalmap
	{L"emissiveMap", 0, 6},	 // emissivemap
	{L"occlusionMap", 0, 7}, // occlusionmap
	{L"roughnessMap", 0, 8}, // roughnessmap
	{L"metallicMap", 0, 9}	 // metallicmap
};

std::vector<const wchar_t*> toPtrVec(const prtx::WStringVector& wsv)
{
	std::vector<const wchar_t*> pw(wsv.size());
	for (size_t i = 0; i < wsv.size(); i++)
		pw[i] = wsv[i].c_str();
	return pw;
}

template <typename T>
std::pair<std::vector<const T*>, std::vector<size_t>> toPtrVec(const std::vector<std::vector<T>>& v)
{
	std::vector<const T*> pv(v.size());
	std::vector<size_t> ps(v.size());
	for (size_t i = 0; i < v.size(); i++)
	{
		pv[i] = v[i].data();
		ps[i] = v[i].size();
	}
	return std::make_pair(pv, ps);
}

// return the highest required uv set (where a valid texture is present)
uint32_t scanValidTextures(const prtx::MaterialPtr& mat)
{
	int8_t highestUVSet = -1;
	for (const auto& t : TEXTURE_UV_MAPPINGS)
	{
		const auto& ta = mat->getTextureArray(t.key);
		if (ta.size() > t.index && ta[t.index]->isValid())
			highestUVSet = std::max(highestUVSet, t.uvSet);
	}
	if (highestUVSet < 0)
		return 0;
	else
		return highestUVSet + 1;
}

// use shape name by default for the actor and if the instance originates from a file we use the file name for better readability
std::wstring createInstanceName(const prtx::EncodePreparator::FinalizedInstance& fi)
{
	const auto geometry = fi.getGeometry();
	const auto uri = geometry->getURI();

	if (uri->isFilePath())
		return uri->getBaseName();

	std::wstring meshName = fi.getShapeName();

	// remove last point similar to the model hierarchy
	if (!meshName.empty() && meshName.back() == '.')
		meshName = meshName.substr(0, meshName.length() - 1);

	return meshName;
}

std::wstring uriToPath(const prtx::TexturePtr& t)
{
	return t->getURI()->wstring();
}

// we blacklist all CGA-style material attribute keys, see prtx/Material.h
// clang-format off
	const std::set<std::wstring> MATERIAL_ATTRIBUTE_BLACKLIST = {
		L"ambient.b",
		L"ambient.g",
		L"ambient.r",
		L"bumpmap.rw",
		L"bumpmap.su",
		L"bumpmap.sv",
		L"bumpmap.tu",
		L"bumpmap.tv",
		L"color.a",
		L"color.b",
		L"color.g",
		L"color.r",
		L"color.rgb",
		L"colormap.rw",
		L"colormap.su",
		L"colormap.sv",
		L"colormap.tu",
		L"colormap.tv",
		L"dirtmap.rw",
		L"dirtmap.su",
		L"dirtmap.sv",
		L"dirtmap.tu",
		L"dirtmap.tv",
		L"normalmap.rw",
		L"normalmap.su",
		L"normalmap.sv",
		L"normalmap.tu",
		L"normalmap.tv",
		L"opacitymap.rw",
		L"opacitymap.su",
		L"opacitymap.sv",
		L"opacitymap.tu",
		L"opacitymap.tv",
		L"specular.b",
		L"specular.g",
		L"specular.r",
		L"specularmap.rw",
		L"specularmap.su",
		L"specularmap.sv",
		L"specularmap.tu",
		L"specularmap.tv",
		L"bumpmap",
		L"colormap",
		L"dirtmap",
		L"normalmap",
		L"opacitymap",
		L"specularmap",
		L"emissive.b",
		L"emissive.g",
		L"emissive.r",
		L"emissivemap.rw",
		L"emissivemap.su",
		L"emissivemap.sv",
		L"emissivemap.tu",
		L"emissivemap.tv",
		L"metallicmap.rw",
		L"metallicmap.su",
		L"metallicmap.sv",
		L"metallicmap.tu",
		L"metallicmap.tv",
		L"occlusionmap.rw",
		L"occlusionmap.su",
		L"occlusionmap.sv",
		L"occlusionmap.tu",
		L"occlusionmap.tv",
		L"roughnessmap.rw",
		L"roughnessmap.su",
		L"roughnessmap.sv",
		L"roughnessmap.tu",
		L"roughnessmap.tv",
		L"emissivemap",
		L"metallicmap",
		L"occlusionmap",
		L"roughnessmap"
	};
// clang-format on

void convertMaterialToAttributeMap(prtx::PRTUtils::AttributeMapBuilderPtr& aBuilder, const prtx::Material& prtxAttr, const prtx::WStringVector& keys)
{
	if (DBG)
		log_debug(L"-- converting material: %1%") % prtxAttr.name();
	for (const auto& key : keys)
	{
		if (MATERIAL_ATTRIBUTE_BLACKLIST.count(key) > 0)
			continue;

		if (DBG)
			log_debug(L"   key: %1%") % key;

		switch (prtxAttr.getType(key))
		{
		case prt::Attributable::PT_BOOL:
			aBuilder->setBool(key.c_str(), prtxAttr.getBool(key) == prtx::PRTX_TRUE);
			break;

		case prt::Attributable::PT_FLOAT:
			aBuilder->setFloat(key.c_str(), prtxAttr.getFloat(key));
			break;

		case prt::Attributable::PT_INT:
			aBuilder->setInt(key.c_str(), prtxAttr.getInt(key));
			break;

		case prt::Attributable::PT_STRING:
		{
			const std::wstring& v = prtxAttr.getString(key); // explicit copy
			aBuilder->setString(key.c_str(), v.c_str());	 // also passing on empty strings
			break;
		}

		case prt::Attributable::PT_BOOL_ARRAY:
		{
			const std::vector<uint8_t>& ba = prtxAttr.getBoolArray(key);
			auto boo = std::unique_ptr<bool[]>(new bool[ba.size()]);
			for (size_t i = 0; i < ba.size(); i++)
				boo[i] = (ba[i] == prtx::PRTX_TRUE);
			aBuilder->setBoolArray(key.c_str(), boo.get(), ba.size());
			break;
		}

		case prt::Attributable::PT_INT_ARRAY:
		{
			const std::vector<int32_t>& array = prtxAttr.getIntArray(key);
			aBuilder->setIntArray(key.c_str(), &array[0], array.size());
			break;
		}

		case prt::Attributable::PT_FLOAT_ARRAY:
		{
			const std::vector<double>& array = prtxAttr.getFloatArray(key);
			aBuilder->setFloatArray(key.c_str(), array.data(), array.size());
			break;
		}

		case prt::Attributable::PT_STRING_ARRAY:
		{
			const prtx::WStringVector& a = prtxAttr.getStringArray(key);
			std::vector<const wchar_t*> pw = toPtrVec(a);
			aBuilder->setStringArray(key.c_str(), pw.data(), pw.size());
			break;
		}

		case prtx::Material::PT_TEXTURE:
		{
			const auto& t = prtxAttr.getTexture(key);
			std::wstring path = uriToPath(t);
			prtx::WStringVector pw;
			pw.push_back(path);
			std::vector<const wchar_t*> ppa = toPtrVec(pw);
			aBuilder->setStringArray(key.c_str(), ppa.data(), 1);
			break;
		}

		case prtx::Material::PT_TEXTURE_ARRAY:
		{
			const auto& ta = prtxAttr.getTextureArray(key);

			prtx::WStringVector pa(ta.size());
			std::transform(ta.begin(), ta.end(), pa.begin(), uriToPath);

			std::vector<const wchar_t*> ppa = toPtrVec(pa);
			aBuilder->setStringArray(key.c_str(), ppa.data(), ppa.size());
			break;
		}

		default:
			if (DBG)
				log_debug(L"ignored atttribute '%s' with type %d") % key % prtxAttr.getType(key);
			break;
		}
	}
}

template <typename F>
void forEachKey(prt::Attributable const* a, F f)
{
	if (a == nullptr)
		return;

	size_t keyCount = 0;
	wchar_t const* const* keys = a->getKeys(&keyCount);

	for (size_t k = 0; k < keyCount; k++)
	{
		wchar_t const* const key = keys[k];
		f(a, key);
	}
}

void forwardGenericAttributes(IUnrealCallbacks* uc, size_t initialShapeIndex, const prtx::InitialShape& initialShape, const prtx::ShapePtr& shape)
{
	forEachKey(initialShape.getAttributeMap(), [&uc, &shape, &initialShapeIndex, &initialShape](prt::Attributable const* /*a*/, wchar_t const* key) {
		switch (shape->getType(key))
		{
		case prtx::Attributable::PT_STRING:
		{
			const auto v = shape->getString(key);
			uc->attrString(initialShapeIndex, shape->getID(), key, v.c_str());
			break;
		}
		case prtx::Attributable::PT_FLOAT:
		{
			const auto v = shape->getFloat(key);
			uc->attrFloat(initialShapeIndex, shape->getID(), key, v);
			break;
		}
		case prtx::Attributable::PT_BOOL:
		{
			const auto v = shape->getBool(key);
			uc->attrBool(initialShapeIndex, shape->getID(), key, (v == prtx::PRTX_TRUE));
			break;
		}
		default:
			break;
		}
	});
}

SerializedGeometry serializeGeometry(const prtx::GeometryPtrVector& geometries, const std::vector<prtx::MaterialPtrVector>& materials)
{
	// PASS 1: scan
	uint32_t numCounts = 0;
	uint32_t numIndices = 0;
	uint32_t maxNumUVSets = 0;
	auto matsIt = materials.cbegin();
	//prt supports up to 10 uv sets (see: https://doc.arcgis.com/en/cityengine/latest/cga/cga-texturing-essential-knowledge.htm)
	std::vector<bool> isUVSetEmptyVector(10, true);
	for (const auto& geo : geometries)
	{
		const prtx::MeshPtrVector& meshes = geo->getMeshes();
		const prtx::MaterialPtrVector& mats = *matsIt;
		auto matIt = mats.cbegin();
		for (const auto& mesh : meshes)
		{
			numCounts += mesh->getFaceCount();
			const auto& vtxCnts = mesh->getFaceVertexCounts();
			numIndices = std::accumulate(vtxCnts.begin(), vtxCnts.end(), numIndices);

			const prtx::MaterialPtr& mat = *matIt;
			const uint32_t requiredUVSetsByMaterial = scanValidTextures(mat);
			maxNumUVSets = std::max(maxNumUVSets, std::max(mesh->getUVSetsCount(), requiredUVSetsByMaterial));

			for (uint32_t uvSet = 0; uvSet < mesh->getUVSetsCount(); uvSet++) {
				if (!mesh->getUVCoords(uvSet).empty())
				{
					isUVSetEmptyVector[uvSet] = false;
				}
			}
			++matIt;
		}
		++matsIt;
	}
	SerializedGeometry sg(numCounts, numIndices, maxNumUVSets);

	// PASS 2: copy
	uint32_t vertexIndexBase = 0u;
	uint32_t normalIndexBase = 0u;
	std::vector<uint32_t> uvIndexBases(maxNumUVSets, 0u);
	for (const auto& geo : geometries)
	{
		const prtx::MeshPtrVector& meshes = geo->getMeshes();
		for (const auto& mesh : meshes)
		{
			// append points
			const prtx::DoubleVector& verts = mesh->getVertexCoords();
			sg.coords.insert(sg.coords.end(), verts.begin(), verts.end());

			// append normals
			const prtx::DoubleVector& norms = mesh->getVertexNormalsCoords();
			sg.normals.insert(sg.normals.end(), norms.begin(), norms.end());

			// append uv sets (uv coords, counts, indices) with special cases:
			// - if mesh has no uv sets but maxNumUVSets is > 0, insert "0" uv face counts to keep in sync
			// - if a uv set is empty for all meshes, leave it empty
			//   (fall back to uv set 0 in the material shader instead of copying them here)
			// - if mesh is missing uv sets that another mesh has, copy uv set 0 to the missing sets
			const uint32_t numUVSets = mesh->getUVSetsCount();
			const prtx::DoubleVector& uvs0 = (numUVSets > 0) ? mesh->getUVCoords(0) : EMPTY_UVS;
			const prtx::IndexVector faceUVCounts0 = (numUVSets > 0) ? mesh->getFaceUVCounts(0) : prtx::IndexVector(mesh->getFaceCount(), 0);
			if (DBG)
				log_debug("-- mesh: numUVSets = %1%") % numUVSets;

			for (uint32_t uvSet = 0; uvSet < sg.uvs.size(); uvSet++)
			{
				if (isUVSetEmptyVector[uvSet])
					continue;

				// append texture coordinates
				const prtx::DoubleVector& uvs = (uvSet < numUVSets) ? mesh->getUVCoords(uvSet) : EMPTY_UVS;
				const auto& src = uvs.empty() ? uvs0 : uvs;
				auto& tgt = sg.uvs[uvSet];
				tgt.insert(tgt.end(), src.begin(), src.end());

				// append uv face counts
				const prtx::IndexVector& faceUVCounts = (uvSet < numUVSets && !uvs.empty()) ? mesh->getFaceUVCounts(uvSet) : faceUVCounts0;
				assert(faceUVCounts.size() == mesh->getFaceCount());
				auto& tgtCnts = sg.uvCounts[uvSet];
				tgtCnts.insert(tgtCnts.end(), faceUVCounts.begin(), faceUVCounts.end());
				if (DBG)
					log_debug("   -- uvset %1%: face counts size = %2%") % uvSet % faceUVCounts.size();

				// append uv vertex indices
				for (uint32_t fi = 0, faceCount = static_cast<uint32_t>(faceUVCounts.size()); fi < faceCount; ++fi)
				{
					const uint32_t* faceUVIdx0 = (numUVSets > 0) ? mesh->getFaceUVIndices(fi, 0) : EMPTY_IDX.data();
					const uint32_t* faceUVIdx = (uvSet < numUVSets && !uvs.empty()) ? mesh->getFaceUVIndices(fi, uvSet) : faceUVIdx0;
					const uint32_t faceUVCnt = faceUVCounts[fi];
					if (DBG)
						log_debug("      fi %1%: faceUVCnt = %2%, faceVtxCnt = %3%") % fi % faceUVCnt % mesh->getFaceVertexCount(fi);
					for (uint32_t vi = 0; vi < faceUVCnt; vi++)
						sg.uvIndices[uvSet].push_back(uvIndexBases[uvSet] + faceUVIdx[vi]);
				}

				uvIndexBases[uvSet] += static_cast<uint32_t>(src.size()) / 2;
			} // for all uv sets

			// append counts and indices for vertices and vertex normals
			for (uint32_t fi = 0, faceCount = mesh->getFaceCount(); fi < faceCount; ++fi)
			{
				const uint32_t vtxCnt = mesh->getFaceVertexCount(fi);
				sg.faceVertexCounts.push_back(vtxCnt);
				const uint32_t* vtxIdx = mesh->getFaceVertexIndices(fi);
				const uint32_t* nrmIdx = mesh->getFaceVertexNormalIndices(fi);
				for (uint32_t vi = 0; vi < vtxCnt; vi++)
				{
					sg.vertexIndices.push_back(vertexIndexBase + vtxIdx[vi]);
					sg.normalIndices.push_back(normalIndexBase + nrmIdx[vi]);
				}
			}

			vertexIndexBase += (uint32_t)verts.size() / 3u;
			normalIndexBase += (uint32_t)norms.size() / 3u;
		} // for all meshes
	}	  // for all geometries

	return sg;
}

void encodeMesh(IUnrealCallbacks* cb, const SerializedGeometry& sg, wchar_t const* name, int32_t prototypeIndex, const std::wstring& uri,
				prtx::GeometryPtrVector geometries, std::vector<prtx::MaterialPtrVector> materials)
{
	auto puvs = toPtrVec(sg.uvs);
	auto puvCounts = toPtrVec(sg.uvCounts);
	auto puvIndices = toPtrVec(sg.uvIndices);

	std::vector<uint32_t> faceRanges;
	AttributeMapNOPtrVectorOwner matAttrMaps;

	auto matIt = materials.cbegin();
	prtx::PRTUtils::AttributeMapBuilderPtr amb(prt::AttributeMapBuilder::create());
	for (const auto& geo : geometries)
	{
		const prtx::MeshPtrVector& meshes = geo->getMeshes();

		for (size_t mi = 0; mi < meshes.size(); mi++)
		{
			const prtx::MeshPtr& m = meshes.at(mi);
			const prtx::MaterialPtr& mat = matIt->at(mi);

			convertMaterialToAttributeMap(amb, *(mat.get()), mat->getKeys());
			matAttrMaps.v.push_back(amb->createAttributeMapAndReset());
			faceRanges.push_back(m->getFaceCount());
		}

		++matIt;
	}

	cb->addMesh(name, prototypeIndex, uri.c_str(), sg.coords.data(), sg.coords.size(), sg.normals.data(), sg.normals.size(),
				sg.faceVertexCounts.data(), sg.faceVertexCounts.size(), sg.vertexIndices.data(), sg.vertexIndices.size(), sg.normalIndices.data(),
				sg.normalIndices.size(),

				puvs.first.data(), puvs.second.data(), puvCounts.first.data(), puvCounts.second.data(), puvIndices.first.data(),
				puvIndices.second.data(), sg.uvs.size(),

				faceRanges.data(), faceRanges.size(), matAttrMaps.v.empty() ? nullptr : matAttrMaps.v.data());
}

const prtx::PRTUtils::AttributeMapPtr convertReportToAttributeMap(const prtx::ReportsPtr& r) {
	prtx::PRTUtils::AttributeMapBuilderPtr amb(prt::AttributeMapBuilder::create());

	for (const auto& b : r->mBools)
		amb->setBool(b.first->c_str(), b.second);
	for (const auto& f : r->mFloats)
		amb->setFloat(f.first->c_str(), f.second);
	for (const auto& s : r->mStrings)
		amb->setString(s.first->c_str(), s.second->c_str());

	return prtx::PRTUtils::AttributeMapPtr{amb->createAttributeMap()};
}
} // namespace

UnrealGeometryEncoder::UnrealGeometryEncoder(const std::wstring& id, const prt::AttributeMap* options, prt::Callbacks* callbacks)
	: prtx::GeometryEncoder(id, options, callbacks)
{
}

void UnrealGeometryEncoder::init(prtx::GenerateContext&)
{
	auto* callbacks = dynamic_cast<IUnrealCallbacks*>(getCallbacks());
	if (callbacks == nullptr)
		throw prtx::StatusException(prt::STATUS_ILLEGAL_CALLBACK_OBJECT);
}

void UnrealGeometryEncoder::encode(prtx::GenerateContext& context, size_t initialShapeIndex)
{
	const prtx::InitialShape& initialShape = *context.getInitialShape(initialShapeIndex);

	IUnrealCallbacks* cb = static_cast<IUnrealCallbacks*>(getCallbacks());

	const bool emitAttrs = getOptions()->getBool(EO_EMIT_ATTRIBUTES);

	prtx::DefaultNamePreparator namePrep;
	prtx::NamePreparator::NamespacePtr nsMesh = namePrep.newNamespace();
	prtx::NamePreparator::NamespacePtr nsMaterial = namePrep.newNamespace();
	prtx::EncodePreparatorPtr encPrep = prtx::EncodePreparator::create(true, namePrep, nsMesh, nsMaterial);

	// generate geometry
	prtx::ReportsAccumulatorPtr reportsAccumulator{prtx::SummarizingReportsAccumulator::create()};
	prtx::ReportingStrategyPtr reportsCollector{prtx::AllShapesReportingStrategy::create(context, initialShapeIndex, reportsAccumulator)};
	prtx::LeafIteratorPtr li = prtx::LeafIterator::create(context, initialShapeIndex);
	for (prtx::ShapePtr shape = li->getNext(); shape; shape = li->getNext())
	{
		encPrep->add(context.getCache(), shape, initialShape.getAttributeMap());
	}

	const prtx::EncodePreparator::PreparationFlags PREP_FLAGS =
		prtx::EncodePreparator::PreparationFlags()
			.instancing(true)
			.meshMerging(prtx::MeshMerging::ALL_OF_SAME_MATERIAL_AND_TYPE)
			.triangulate(false)
			.processHoles(prtx::HoleProcessor::TRIANGULATE_FACES_WITH_HOLES)
			.mergeVertices(true)
			.cleanupVertexNormals(true)
			.cleanupUVs(true)
			.processVertexNormals(prtx::VertexNormalProcessor::SET_MISSING_TO_FACE_NORMALS)
			.indexSharing(prtx::EncodePreparator::PreparationFlags::INDICES_SEPARATE_FOR_ALL_VERTEX_ATTRIBUTES);

	prtx::EncodePreparator::InstanceVector instances;
	encPrep->fetchFinalizedInstances(instances, PREP_FLAGS);
	convertGeometry(initialShape, instances, cb);

	if (emitAttrs) {
		const prtx::ReportsPtr& reports = reportsCollector->getReports();
		if (reports) {
			prtx::PRTUtils::AttributeMapPtr reportMap = convertReportToAttributeMap(reports);
			cb->addReport(reportMap.get());
		}
	}
}

void UnrealGeometryEncoder::convertGeometry(const prtx::InitialShape& initialShape, const prtx::EncodePreparator::InstanceVector& instances,
											IUnrealCallbacks* cb) const
{
	std::set<int> serializedPrototypes;

	prtx::GeometryPtrVector geometries;
	std::vector<prtx::MaterialPtrVector> materials;
	prtx::PRTUtils::AttributeMapBuilderPtr instanceMatAmb(prt::AttributeMapBuilder::create());
	for (const auto& inst : instances)
	{
		if (inst.getPrototypeIndex() != -1)
		{
			const prtx::MaterialPtrVector& instMaterials = inst.getMaterials();
			const prtx::GeometryPtr& instGeom = inst.getGeometry();

			AttributeMapNOPtrVectorOwner instMaterialsAttributeMap;

			if (serializedPrototypes.find(inst.getPrototypeIndex()) == serializedPrototypes.end())
			{
				const std::wstring uri = instGeom->getURI()->wstring();
				const SerializedGeometry sg = serializeGeometry({instGeom}, {instMaterials});
				const std::wstring instName = createInstanceName(inst);

				encodeMesh(cb, sg, instName.c_str(), inst.getPrototypeIndex(), uri, {instGeom}, {instMaterials});

				serializedPrototypes.insert(inst.getPrototypeIndex());
			}

			const prtx::MeshPtrVector& meshes = instGeom->getMeshes();
			for (size_t mi = 0; mi < meshes.size(); mi++)
			{
				const prtx::MaterialPtr& mat = instMaterials[mi];

				convertMaterialToAttributeMap(instanceMatAmb, *(mat.get()), mat->getKeys());
				instMaterialsAttributeMap.v.push_back(instanceMatAmb->createAttributeMapAndReset());
			}

			cb->addInstance(inst.getPrototypeIndex(), inst.getTransformation().data(), instMaterialsAttributeMap.v.data(),
							instMaterialsAttributeMap.v.size());
		}
		else
		{
			geometries.push_back(inst.getGeometry());
			materials.push_back(inst.getMaterials());
		}
	}

	if (geometries.size() > 0)
	{
		const SerializedGeometry sg = serializeGeometry(geometries, materials);
		encodeMesh(cb, sg, initialShape.getName(), -1, L"", geometries, materials);
	}

	if (DBG)
		log_debug(L"UnrealGeometryEncoder::convertGeometry: end");
}

void UnrealGeometryEncoder::finish(prtx::GenerateContext& /*context*/) {}

UnrealGeometryEncoderFactory* UnrealGeometryEncoderFactory::createInstance()
{
	prtx::EncoderInfoBuilder encoderInfoBuilder;

	encoderInfoBuilder.setID(UNREAL_GEOMETRY_ENCODER_ID);
	encoderInfoBuilder.setName(UNREAL_GEOMETRY_ENCODER_NAME);
	encoderInfoBuilder.setDescription(UNREAL_GEOMETRY_ENCODER_DESCRIPTION);
	encoderInfoBuilder.setType(prt::CT_GEOMETRY);

	prtx::PRTUtils::AttributeMapBuilderPtr amb(prt::AttributeMapBuilder::create());
	amb->setBool(EO_EMIT_ATTRIBUTES, true);
	amb->setBool(EO_EMIT_MATERIALS, true);
	encoderInfoBuilder.setDefaultOptions(amb->createAttributeMap());

	return new UnrealGeometryEncoderFactory(encoderInfoBuilder.create());
}
