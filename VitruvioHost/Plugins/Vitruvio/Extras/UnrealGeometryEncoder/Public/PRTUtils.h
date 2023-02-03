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

#pragma warning(push)
#pragma warning(disable : 4263 4264)
#include "prt/API.h"
#pragma warning(pop)

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#if PLATFORM_WINDOWS

#include "Windows/AllowWindowsPlatformTypes.h"

#include "Windows.h"

#include "Windows/HideWindowsPlatformTypes.h"

#endif

namespace prtu
{
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

constexpr wchar_t STYLE_DELIMITER = L'$';
constexpr wchar_t IMPORT_DELIMITER = L'.';

inline std::wstring getStyle(const std::wstring& fqRuleName)
{
	const auto sepPos = fqRuleName.find(STYLE_DELIMITER);
	if (sepPos == std::wstring::npos || sepPos == 0)
		return {};
	return fqRuleName.substr(0, sepPos);
}

inline std::wstring removePrefix(const std::wstring& fqRuleName, wchar_t delim)
{
	const auto sepPos = fqRuleName.rfind(delim);
	if (sepPos == std::wstring::npos)
		return fqRuleName;
	if (sepPos == fqRuleName.length() - 1)
		return {};
	if (fqRuleName.length() <= 1)
		return {};
	return fqRuleName.substr(sepPos + 1);
}

inline std::wstring removeStyle(const std::wstring& fqRuleName)
{
	return removePrefix(fqRuleName, STYLE_DELIMITER);
}

inline std::wstring removeImport(const std::wstring& fqRuleName)
{
	return removePrefix(fqRuleName, IMPORT_DELIMITER);
}

inline std::wstring getFullImportPath(const std::wstring& fqRuleName)
{
	std::wstring fullPath = removePrefix(fqRuleName, STYLE_DELIMITER);
	const auto sepPos = fullPath.rfind(IMPORT_DELIMITER);
	if (sepPos == std::wstring::npos || sepPos == 0)
		return {};
	return fullPath.substr(0, sepPos);
}

inline AttributeMapUPtr createValidatedOptions(const wchar_t* encID, const prt::AttributeMap* unvalidatedOptions = nullptr)
{
	const EncoderInfoUPtr encInfo(prt::createEncoderInfo(encID));
	if (!encInfo)
		return {};
	const prt::AttributeMap* validatedOptions = nullptr;
	const prt::AttributeMap* optionStates = nullptr;
	const prt::Status s = encInfo->createValidatedOptionsAndStates(unvalidatedOptions, &validatedOptions, &optionStates);
	if (optionStates != nullptr)
		optionStates->destroy(); // we don't need that atm
	if (s != prt::STATUS_OK)
		return {};
	return AttributeMapUPtr(validatedOptions);
}

template <typename CO, typename CI, typename AF>
std::basic_string<CO> stringConversionWrapper(AF apiFunc, const std::basic_string<CI>& inputString)
{
	std::vector<CO> temp(2 * inputString.size(), 0);
	size_t size = temp.size();
	prt::Status status = prt::STATUS_OK;
	apiFunc(inputString.c_str(), temp.data(), &size, &status);
	if (status != prt::STATUS_OK)
		throw std::runtime_error(prt::getStatusDescription(status));
	if (size > temp.size())
	{
		temp.resize(size);
		apiFunc(inputString.c_str(), temp.data(), &size, &status);
		if (status != prt::STATUS_OK)
			throw std::runtime_error(prt::getStatusDescription(status));
	}
	return std::basic_string<CO>(temp.data());
}

inline std::string toOSNarrowFromUTF16(const std::wstring& u16String)
{
	return stringConversionWrapper<char, wchar_t>(prt::StringUtils::toOSNarrowFromUTF16, u16String);
}

inline std::wstring toUTF16FromOSNarrow(const std::string& osString)
{
	return stringConversionWrapper<wchar_t, char>(prt::StringUtils::toUTF16FromOSNarrow, osString);
}

inline std::wstring toUTF16FromUTF8(const std::string& u8String)
{
	return stringConversionWrapper<wchar_t, char>(prt::StringUtils::toUTF16FromUTF8, u8String);
}

inline std::string toUTF8FromUTF16(const std::wstring& u16String)
{
	return stringConversionWrapper<char, wchar_t>(prt::StringUtils::toUTF8FromUTF16, u16String);
}

inline std::string percentEncode(const std::string& utf8String)
{
	return stringConversionWrapper<char, char>(prt::StringUtils::percentEncode, utf8String);
}

inline std::wstring toFileURI(const std::wstring& p)
{
#if PLATFORM_WINDOWS
	static const std::wstring schema = L"file:/";
#else
	static const std::wstring schema = L"file:";
#endif
	std::string utf8Path = prtu::toUTF8FromUTF16(p);
	std::string pecString = percentEncode(utf8Path);
	std::wstring u16String = toUTF16FromUTF8(pecString);
	return schema + u16String;
}

inline std::wstring temp_directory_path()
{
#if PLATFORM_WINDOWS
	wchar_t lpTempPathBuffer[MAX_PATH];
	DWORD dwRetVal = GetTempPathW(MAX_PATH, lpTempPathBuffer);
	if (dwRetVal > MAX_PATH || (dwRetVal == 0))
	{
		return L".\tmp";
	}
	else
	{
		return std::wstring(lpTempPathBuffer);
	}

#else

	char const* folder = getenv("TMPDIR");
	if (folder == nullptr)
	{
		folder = getenv("TMP");
		if (folder == nullptr)
		{
			folder = getenv("TEMP");
			if (folder == nullptr)
			{
				folder = getenv("TEMPDIR");
				if (folder == nullptr)
					folder = "/tmp";
			}
		}
	}

	return toUTF16FromOSNarrow(std::string(folder));
#endif
}
} // namespace prtu
