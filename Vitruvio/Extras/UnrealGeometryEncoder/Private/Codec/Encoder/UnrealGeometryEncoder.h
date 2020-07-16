// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#pragma warning(push)
#pragma warning(disable : 4263 4264)
#include "prtx/EncodePreparator.h"
#include "prtx/Encoder.h"
#include "prtx/EncoderFactory.h"
#include "prtx/EncoderInfoBuilder.h"
#include "prtx/PRTUtils.h"
#include "prtx/ResolveMap.h"
#include "prtx/Singleton.h"
#include "prtx/StreamAdaptor.h"
#include "prtx/StreamAdaptorFactory.h"

#include "prt/ContentType.h"
#include "prt/InitialShape.h"
#pragma warning(pop)

#include "Codec/CodecMain.h"

#include <iostream>
#include <stdexcept>
#include <string>

class IUnrealCallbacks;

using InstanceVectorPtr = std::shared_ptr<prtx::EncodePreparator::InstanceVector>;

class UnrealGeometryEncoder final : public prtx::GeometryEncoder
{
public:
	UnrealGeometryEncoder(const std::wstring& id, const prt::AttributeMap* options, prt::Callbacks* callbacks);
	~UnrealGeometryEncoder() override = default;

public:
	void init(prtx::GenerateContext& context) override;
	void encode(prtx::GenerateContext& context, size_t initialShapeIndex) override;
	void finish(prtx::GenerateContext& context) override;

private:
	void convertGeometry(const prtx::InitialShape& initialShape, const prtx::EncodePreparator::InstanceVector& instances,
						 IUnrealCallbacks* callbacks) const;
};

class UnrealGeometryEncoderFactory final : public prtx::EncoderFactory, public prtx::Singleton<UnrealGeometryEncoderFactory>
{
public:
	static UnrealGeometryEncoderFactory* createInstance();

	explicit UnrealGeometryEncoderFactory(const prt::EncoderInfo* info) : prtx::EncoderFactory(info) {}
	~UnrealGeometryEncoderFactory() override = default;

	UnrealGeometryEncoder* create(const prt::AttributeMap* options, prt::Callbacks* callbacks) const override
	{
		return new UnrealGeometryEncoder(getID(), options, callbacks);
	}
};
