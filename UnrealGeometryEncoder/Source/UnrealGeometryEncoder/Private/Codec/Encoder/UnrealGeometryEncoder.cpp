#include "UnrealGeometryEncoder.h"
#include "IUnrealCallbacks.h"

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

#include <algorithm>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>
#include <set>

namespace
{

	constexpr bool DBG = false;

	constexpr const wchar_t* ENC_NAME = L"Unreal Encoder";
	constexpr const wchar_t* ENC_DESCRIPTION = L"Encodes geometry into Unreal geometry.";

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

	struct AttributeMapNOPtrVectorOwner {
		AttributeMapNOPtrVector v;
		~AttributeMapNOPtrVectorOwner() {
			for (const auto& m : v) {
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

	const std::vector<TextureUVMapping> TEXTURE_UV_MAPPINGS = []() -> std::vector<TextureUVMapping> {
		return {
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
	}();

	std::vector<const wchar_t*> toPtrVec(const prtx::WStringVector& wsv)
	{
		std::vector<const wchar_t*> pw(wsv.size());
		for (size_t i = 0; i < wsv.size(); i++)
			pw[i] = wsv[i].c_str();
		return pw;
	}

	template <typename T> std::pair<std::vector<const T*>, std::vector<size_t>> toPtrVec(const std::vector<std::vector<T>>& v)
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

	std::wstring uriToPath(const prtx::TexturePtr& t) {
		return t->getURI()->getPath();
	}

	// we blacklist all CGA-style material attribute keys, see prtx/Material.h
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
	        L"opacitymap.mode",
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


	void convertMaterialToAttributeMap(prtx::PRTUtils::AttributeMapBuilderPtr& aBuilder, const prtx::Material& prtxAttr, const prtx::WStringVector& keys) {
	if (DBG)
		log_debug(L"-- converting material: %1%") % prtxAttr.name();
	for (const auto& key : keys) {
		if (MATERIAL_ATTRIBUTE_BLACKLIST.count(key) > 0)
			continue;

		if (DBG)
			log_debug(L"   key: %1%") % key;

		switch (prtxAttr.getType(key)) {
			case prt::Attributable::PT_BOOL:
				aBuilder->setBool(key.c_str(), prtxAttr.getBool(key) == prtx::PRTX_TRUE);
				break;

			case prt::Attributable::PT_FLOAT:
				aBuilder->setFloat(key.c_str(), prtxAttr.getFloat(key));
				break;

			case prt::Attributable::PT_INT:
				aBuilder->setInt(key.c_str(), prtxAttr.getInt(key));
				break;

			case prt::Attributable::PT_STRING: {
				const std::wstring& v = prtxAttr.getString(key); // explicit copy
				aBuilder->setString(key.c_str(), v.c_str());     // also passing on empty strings
				break;
			}

			case prt::Attributable::PT_BOOL_ARRAY: {
				const std::vector<uint8_t>& ba = prtxAttr.getBoolArray(key);
				auto boo = std::unique_ptr<bool[]>(new bool[ba.size()]);
				for (size_t i = 0; i < ba.size(); i++)
					boo[i] = (ba[i] == prtx::PRTX_TRUE);
				aBuilder->setBoolArray(key.c_str(), boo.get(), ba.size());
				break;
			}

			case prt::Attributable::PT_INT_ARRAY: {
				const std::vector<int32_t>& array = prtxAttr.getIntArray(key);
				aBuilder->setIntArray(key.c_str(), &array[0], array.size());
				break;
			}

			case prt::Attributable::PT_FLOAT_ARRAY: {
				const std::vector<double>& array = prtxAttr.getFloatArray(key);
				aBuilder->setFloatArray(key.c_str(), array.data(), array.size());
				break;
			}

			case prt::Attributable::PT_STRING_ARRAY: {
				const prtx::WStringVector& a = prtxAttr.getStringArray(key);
				std::vector<const wchar_t*> pw = toPtrVec(a);
				aBuilder->setStringArray(key.c_str(), pw.data(), pw.size());
				break;
			}

			case prtx::Material::PT_TEXTURE: {
				const auto& t = prtxAttr.getTexture(key);
				const std::wstring p = uriToPath(t);
				aBuilder->setString(key.c_str(), p.c_str());
				break;
			}

			case prtx::Material::PT_TEXTURE_ARRAY: {
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
	
	SerializedGeometry serializeGeometry(const prtx::GeometryPtr& geo, const prtx::MaterialPtrVector& materials)
	{
		// PASS 1: scan
		uint32_t numCounts = 0;
		uint32_t numIndices = 0;
		uint32_t maxNumUVSets = 0;
		const prtx::MeshPtrVector& meshes = geo->getMeshes();
		auto matIt = materials.cbegin();
		for (const auto& mesh : meshes)
		{
			numCounts += mesh->getFaceCount();
			const auto& vtxCnts = mesh->getFaceVertexCounts();
			numIndices = std::accumulate(vtxCnts.begin(), vtxCnts.end(), numIndices);

			const prtx::MaterialPtr& mat = *matIt;
			const uint32_t requiredUVSetsByMaterial = scanValidTextures(mat);
			maxNumUVSets = std::max(maxNumUVSets, std::max(mesh->getUVSetsCount(), requiredUVSetsByMaterial));
			++matIt;
		}
		SerializedGeometry sg(numCounts, numIndices, maxNumUVSets);

		// PASS 2: copy
		uint32_t vertexIndexBase = 0u;
		uint32_t normalIndexBase = 0u;
		std::vector<uint32_t> uvIndexBases(maxNumUVSets, 0u);
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
			// - if mesh has less uv sets than maxNumUVSets, copy uv set 0 to the missing higher sets
			const uint32_t numUVSets = mesh->getUVSetsCount();
			const prtx::DoubleVector& uvs0 = (numUVSets > 0) ? mesh->getUVCoords(0) : EMPTY_UVS;
			const prtx::IndexVector faceUVCounts0 = (numUVSets > 0) ? mesh->getFaceUVCounts(0) : prtx::IndexVector(mesh->getFaceCount(), 0);
			if (DBG)
				log_debug("-- mesh: numUVSets = %1%") % numUVSets;

			for (uint32_t uvSet = 0; uvSet < sg.uvs.size(); uvSet++)
			{
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

			vertexIndexBase += static_cast<uint32_t>(verts.size()) / 3u;
			normalIndexBase += static_cast<uint32_t>(norms.size()) / 3u;
		} // for all meshes

		return sg;
	}

	template <typename F> void forEachKey(prt::Attributable const* a, F f)
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
		forEachKey(initialShape.getAttributeMap(), [&uc, &shape, &initialShapeIndex, &initialShape](prt::Attributable const* a, wchar_t const* key) {
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
} // namespace

UnrealGeometryEncoder::UnrealGeometryEncoder(const std::wstring& id, const prt::AttributeMap* options, prt::Callbacks* callbacks) : prtx::GeometryEncoder(id, options, callbacks)
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
	prtx::ReportsAccumulatorPtr reportsAccumulator{prtx::WriteFirstReportsAccumulator::create()};
	prtx::ReportingStrategyPtr reportsCollector{prtx::LeafShapeReportingStrategy::create(context, initialShapeIndex, reportsAccumulator)};
	prtx::LeafIteratorPtr li = prtx::LeafIterator::create(context, initialShapeIndex);
	for (prtx::ShapePtr shape = li->getNext(); shape; shape = li->getNext())
	{
		prtx::ReportsPtr r = reportsCollector->getReports(shape->getID());
		encPrep->add(context.getCache(), shape, initialShape.getAttributeMap(), r);

		// get final values of generic attributes
		if (emitAttrs)
			forwardGenericAttributes(cb, initialShapeIndex, initialShape, shape);
	}

	const prtx::EncodePreparator::PreparationFlags PREP_FLAGS = prtx::EncodePreparator::PreparationFlags()
		.instancing(true)
		.mergeByMaterial(true)
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
}

void UnrealGeometryEncoder::convertGeometry(const prtx::InitialShape& initialShape, const prtx::EncodePreparator::InstanceVector& instances, IUnrealCallbacks* cb) const
{
	std::set<int> serialized;

	for (const auto& inst : instances)
	{
		if (serialized.find(inst.getPrototypeIndex()) == serialized.end())
		{
			const prtx::GeometryPtr& geo = inst.getGeometry();
			const prtx::MaterialPtrVector& materials = inst.getMaterials();
			const SerializedGeometry sg = serializeGeometry(geo, materials);

			auto puvs = toPtrVec(sg.uvs);
			auto puvCounts = toPtrVec(sg.uvCounts);
			auto puvIndices = toPtrVec(sg.uvIndices);

			uint32_t faceCount = 0;
			std::vector<uint32_t> faceRanges;
			AttributeMapNOPtrVectorOwner matAttrMaps;

			prtx::PRTUtils::AttributeMapBuilderPtr amb(prt::AttributeMapBuilder::create());

			const prtx::MeshPtrVector& meshes = geo->getMeshes();

			for (size_t mi = 0; mi < meshes.size(); mi++) {
				const prtx::MeshPtr& m = meshes.at(mi);
				const prtx::MaterialPtr& mat = materials[mi];

				faceCount += m->getFaceCount();
				faceRanges.push_back(faceCount);

				convertMaterialToAttributeMap(amb, *(mat.get()), mat->getKeys());
				matAttrMaps.v.push_back(amb->createAttributeMapAndReset());
			}

			cb->addMesh(initialShape.getName(), inst.getPrototypeIndex(), sg.coords.data(), sg.coords.size(), sg.normals.data(), sg.normals.size(), sg.faceVertexCounts.data(), sg.faceVertexCounts.size(),
						sg.vertexIndices.data(), sg.vertexIndices.size(), sg.normalIndices.data(), sg.normalIndices.size(),
						
						puvs.first.data(), puvs.second.data(), puvCounts.first.data(), puvCounts.second.data(), puvIndices.first.data(), puvIndices.second.data(), sg.uvs.size(),
						
						faceRanges.data(), faceRanges.size(), matAttrMaps.v.empty() ? nullptr : matAttrMaps.v.data()
			);

			serialized.insert(inst.getPrototypeIndex());
		}

		if (inst.getPrototypeIndex() != -1) 
			cb->addInstance(inst.getPrototypeIndex(), inst.getTransformation().data());
	}

	if (DBG)
		log_debug(L"UnrealGeometryEncoder::convertGeometry: end");
}

void UnrealGeometryEncoder::finish(prtx::GenerateContext& /*context*/)
{
}

UnrealGeometryEncoderFactory* UnrealGeometryEncoderFactory::createInstance()
{
	prtx::EncoderInfoBuilder encoderInfoBuilder;

	encoderInfoBuilder.setID(ENCODER_ID_UnrealGeometry);
	encoderInfoBuilder.setName(ENC_NAME);
	encoderInfoBuilder.setDescription(ENC_DESCRIPTION);
	encoderInfoBuilder.setType(prt::CT_GEOMETRY);

	prtx::PRTUtils::AttributeMapBuilderPtr amb(prt::AttributeMapBuilder::create());
	amb->setBool(EO_EMIT_ATTRIBUTES, true);
	amb->setBool(EO_EMIT_MATERIALS, true);
	encoderInfoBuilder.setDefaultOptions(amb->createAttributeMap());

	return new UnrealGeometryEncoderFactory(encoderInfoBuilder.create());
}