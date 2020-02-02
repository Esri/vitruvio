#pragma once

#include "prt/API.h"
#include "PRTTypes.h"

#include <memory>
#include <string>
#include <vector>

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
