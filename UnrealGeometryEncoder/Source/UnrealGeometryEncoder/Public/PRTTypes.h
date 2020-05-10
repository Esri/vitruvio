// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "prt/API.h"
#include "prt/ResolveMap.h"
#include "prt/RuleFileInfo.h"
#include "prt/StringUtils.h"

#include <memory>
#include <vector>

struct PRTDestroyer
{
	void operator()(prt::Object const* p)
	{
		if (p != nullptr)
			p->destroy();
	}
};

using ResolveMapUPtr = std::unique_ptr<const prt::ResolveMap, PRTDestroyer>;
using RuleFileInfoPtr = std::shared_ptr<const prt::RuleFileInfo>;
using ObjectUPtr = std::unique_ptr<const prt::Object, PRTDestroyer>;
using InitialShapeNOPtrVector = std::vector<const prt::InitialShape*>;
using AttributeMapNOPtrVector = std::vector<const prt::AttributeMap*>;
using CacheObjectUPtr = std::unique_ptr<prt::CacheObject, PRTDestroyer>;
using AttributeMapUPtr = std::unique_ptr<const prt::AttributeMap, PRTDestroyer>;
using AttributeMapVector = std::vector<AttributeMapUPtr>;
using AttributeMapBuilderUPtr = std::unique_ptr<prt::AttributeMapBuilder, PRTDestroyer>;
using AttributeMapBuilderVector = std::vector<AttributeMapBuilderUPtr>;
using InitialShapePtr = std::shared_ptr<const prt::InitialShape>;
using InitialShapeUPtr = std::unique_ptr<const prt::InitialShape, PRTDestroyer>;
using InitialShapeBuilderPtr = std::shared_ptr<const prt::InitialShapeBuilder>;
using InitialShapeBuilderUPtr = std::unique_ptr<prt::InitialShapeBuilder, PRTDestroyer>;
using InitialShapeBuilderVector = std::vector<InitialShapeBuilderUPtr>;
using ResolveMapBuilderUPtr = std::unique_ptr<prt::ResolveMapBuilder, PRTDestroyer>;
using RuleFileInfoUPtr = std::unique_ptr<const prt::RuleFileInfo, PRTDestroyer>;
using EncoderInfoUPtr = std::unique_ptr<const prt::EncoderInfo, PRTDestroyer>;
using OcclusionSetUPtr = std::unique_ptr<prt::OcclusionSet, PRTDestroyer>;
using ResolveMapSPtr = std::shared_ptr<prt::ResolveMap const>;
