#include "UnrealPRTUtils.h"

#include "prt/API.h"
#include "prt/StringUtils.h"

#include <stdexcept>
#include <string>
#include <vector>

#ifdef PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace prtu
{

	AttributeMapUPtr createValidatedOptions(const wchar_t* encID, const prt::AttributeMap* unvalidatedOptions)
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

	std::wstring temp_directory_path()
	{
#ifdef PLATFORM_WINDOWS
		DWORD dwRetVal = 0;
		wchar_t lpTempPathBuffer[MAX_PATH];

		dwRetVal = GetTempPathW(MAX_PATH, lpTempPathBuffer);
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

	template <typename CO, typename CI, typename AF> std::basic_string<CO> stringConversionWrapper(AF apiFunc, const std::basic_string<CI>& inputString)
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

	std::string toOSNarrowFromUTF16(const std::wstring& u16String)
	{
		return stringConversionWrapper<char, wchar_t>(prt::StringUtils::toOSNarrowFromUTF16, u16String);
	}

	std::wstring toUTF16FromOSNarrow(const std::string& osString)
	{
		return stringConversionWrapper<wchar_t, char>(prt::StringUtils::toUTF16FromOSNarrow, osString);
	}

	std::wstring toUTF16FromUTF8(const std::string& u8String)
	{
		return stringConversionWrapper<wchar_t, char>(prt::StringUtils::toUTF16FromUTF8, u8String);
	}

	std::string toUTF8FromUTF16(const std::wstring& u16String)
	{
		return stringConversionWrapper<char, wchar_t>(prt::StringUtils::toUTF8FromUTF16, u16String);
	}

	std::string percentEncode(const std::string& utf8String)
	{
		return stringConversionWrapper<char, char>(prt::StringUtils::percentEncode, utf8String);
	}

	std::wstring toFileURI(const std::wstring& p)
	{
#ifdef PLATFORM_WINDOWS
		static const std::wstring schema = L"file:/";
#else
		static const std::wstring schema = L"file:";
#endif
		std::string utf8Path = prtu::toUTF8FromUTF16(p);
		std::string pecString = percentEncode(utf8Path);
		std::wstring u16String = toUTF16FromUTF8(pecString);
		return schema + u16String;
	}

} // namespace prtu