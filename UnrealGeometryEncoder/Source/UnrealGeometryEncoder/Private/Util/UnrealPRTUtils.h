#pragma once

#include "prt/API.h"
#include "prt/ResolveMap.h"
#include "prt/RuleFileInfo.h"
#include "prt/StringUtils.h"

#include <memory>
#include <string>
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

namespace prtu
{

	// TODO this most likely won't work on Android devices
	std::wstring temp_directory_path();

	// TODO maybe can be replaced with Unreal string conversion
	std::string toOSNarrowFromUTF16(const std::wstring& u16String);
	std::wstring toUTF16FromOSNarrow(const std::string& osString);
	std::wstring toUTF16FromUTF8(const std::string& u8String);
	std::string toUTF8FromUTF16(const std::wstring& u16String);
	std::wstring toFileURI(const std::wstring& p);

	AttributeMapUPtr createValidatedOptions(const wchar_t* encID, const prt::AttributeMap* unvalidatedOptions = nullptr);

	inline std::wstring getRuleFileEntry(const ResolveMapSPtr& resolveMap)
	{
		const std::wstring sCGB(L".cgb");

		size_t nKeys;
		wchar_t const* const* keys = resolveMap->getKeys(&nKeys);
		for (size_t k = 0; k < nKeys; k++)
		{
			const std::wstring key(keys[k]);
			if (std::equal(sCGB.rbegin(), sCGB.rend(), key.rbegin()))
				return key;
		}

		return {};
	}

	constexpr const wchar_t* ANNOT_START_RULE = L"@StartRule";

	inline std::wstring detectStartRule(const RuleFileInfoUPtr& ruleFileInfo)
	{
		for (size_t r = 0; r < ruleFileInfo->getNumRules(); r++)
		{
			const auto* rule = ruleFileInfo->getRule(r);

			// start rules must not have any parameters
			if (rule->getNumParameters() > 0)
				continue;

			for (size_t a = 0; a < rule->getNumAnnotations(); a++)
			{
				if (std::wcscmp(rule->getAnnotation(a)->getName(), ANNOT_START_RULE) == 0)
				{
					return rule->getName();
				}
			}
		}
		return {};
	}

} // namespace prtu
