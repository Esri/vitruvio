/* Copyright 2021 Esri
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

#include "AnnotationParsing.h"

#include "PRTUtils.h"

namespace
{
constexpr const wchar_t* ANNOT_RANGE = L"@Range";
constexpr const wchar_t* ANNOT_ENUM = L"@Enum";
constexpr const wchar_t* ANNOT_HIDDEN = L"@Hidden";
constexpr const wchar_t* ANNOT_COLOR = L"@Color";
constexpr const wchar_t* ANNOT_DIR = L"@Directory";
constexpr const wchar_t* ANNOT_FILE = L"@File";
constexpr const wchar_t* ANNOT_ORDER = L"@Order";
constexpr const wchar_t* ANNOT_GROUP = L"@Group";

constexpr const wchar_t* NULL_KEY = L"#NULL#";
constexpr const wchar_t* MIN_KEY = L"min";
constexpr const wchar_t* MAX_KEY = L"max";
constexpr const wchar_t* STEP_SIZE_KEY = L"stepsize";
constexpr const wchar_t* RESTRICTED_KEY = L"restricted";

bool AreArgumentsOfSameType(const prt::Annotation* Annotation, prt::AnnotationArgumentType& OutType)
{
	if (Annotation->getNumArguments() == 0)
	{
		OutType = prt::AAT_UNKNOWN;
		return true;
	}

	OutType = Annotation->getArgument(0)->getType();

	for (size_t ArgumentIndex = 1; ArgumentIndex < Annotation->getNumArguments(); ArgumentIndex++)
	{
		if (OutType != Annotation->getArgument(ArgumentIndex)->getType())
		{
			OutType = prt::AAT_UNKNOWN;
			return false;
		}
	}
	return true;
}

void ParseValue(const prt::AnnotationArgument* Argument, double& Result)
{
	Result = Argument->getFloat();
}

void ParseValue(const prt::AnnotationArgument* Argument, FString& Result)
{
	Result = FString(WCHAR_TO_TCHAR(Argument->getStr()));
}

template <typename T, typename V>
T* ParseEnumAnnotation(const prt::Annotation* Annotation, UObject* const Outer)
{
	prt::AnnotationArgumentType Type = prt::AAT_UNKNOWN;

	auto Result = NewObject<T>(Outer);

	for (size_t ArgumentIndex = 0; ArgumentIndex < Annotation->getNumArguments(); ArgumentIndex++)
	{
		const wchar_t* Key = Annotation->getArgument(ArgumentIndex)->getKey();
		if (std::wcscmp(Key, NULL_KEY) != 0)
		{
			if (std::wcscmp(Key, RESTRICTED_KEY) == 0)
			{
				Result->Restricted = Annotation->getArgument(ArgumentIndex)->getBool();
			}
			continue;
		}

		V Val;
		ParseValue(Annotation->getArgument(ArgumentIndex), Val);
		Result->Values.Add(Val);
	}
	return Result;
}

URangeAnnotation* ParseRangeAnnotation(const prt::Annotation* Annotation, UObject* const Outer)
{
	auto Result = NewObject<URangeAnnotation>(Outer);
	Result->StepSize = 0.1;

	for (int ArgIndex = 0; ArgIndex < Annotation->getNumArguments(); ArgIndex++)
	{
		const prt::AnnotationArgument* Argument = Annotation->getArgument(ArgIndex);
		const wchar_t* Key = Argument->getKey();

		if (std::wcscmp(Key, MIN_KEY) == 0)
		{
			Result->Min = Argument->getFloat();
			Result->HasMin = true;
		}
		else if (std::wcscmp(Key, MAX_KEY) == 0)
		{
			Result->Max = Argument->getFloat();
			Result->HasMax = true;
		}
		else if (std::wcscmp(Key, STEP_SIZE_KEY) == 0)
		{
			Result->StepSize = Argument->getFloat();
		}
		else if (std::wcscmp(Key, RESTRICTED_KEY) == 0)
		{
			Result->Restricted = Argument->getBool();
		}
	}
	return Result;
}

UFilesystemAnnotation* ParseFileAnnotation(const prt::Annotation* Annotation, UObject* const Outer)
{
	FString Extensions;
	for (size_t ArgumentIndex = 0; ArgumentIndex < Annotation->getNumArguments(); ArgumentIndex++)
	{
		if (Annotation->getArgument(ArgumentIndex)->getType() == prt::AAT_STR)
		{
			Extensions += WCHAR_TO_TCHAR(Annotation->getArgument(ArgumentIndex)->getStr());
			Extensions += TEXT(" (*.");
			Extensions += WCHAR_TO_TCHAR(Annotation->getArgument(ArgumentIndex)->getStr());
			Extensions += TEXT(");");
		}
	}
	Extensions += TEXT("All Files (*.*)");

	auto Result = NewObject<UFilesystemAnnotation>(Outer);
	Result->Mode = EFilesystemMode::File;
	Result->Extensions = Extensions;
	return Result;
}

int ParseOrder(const prt::Annotation* Annotation)
{
	return 0;
}

void ParseGroups(const prt::Annotation* Annotation, URuleAttribute& InAttribute)
{
	for (int AnnotationIndex = 0; AnnotationIndex < Annotation->getNumArguments(); AnnotationIndex++)
	{
		if (Annotation->getArgument(AnnotationIndex)->getType() == prt::AAT_STR)
		{
			InAttribute.Groups.Add(WCHAR_TO_TCHAR(Annotation->getArgument(AnnotationIndex)->getStr()));
		}
		else if (AnnotationIndex == Annotation->getNumArguments() - 1 && Annotation->getArgument(AnnotationIndex)->getType() == prt::AAT_FLOAT)
		{
			InAttribute.GroupOrder = static_cast<int>(Annotation->getArgument(AnnotationIndex)->getFloat());
		}
	}
}
} // namespace

namespace Vitruvio
{
void ParseAttributeAnnotations(const prt::RuleFileInfo::Entry* AttributeInfo, URuleAttribute& InAttribute, UObject* const Outer)
{
	for (size_t AnnotationIndex = 0; AnnotationIndex < AttributeInfo->getNumAnnotations(); ++AnnotationIndex)
	{
		const prt::Annotation* CEAnnotation = AttributeInfo->getAnnotation(AnnotationIndex);

		const wchar_t* Name = CEAnnotation->getName();
		if (std::wcscmp(Name, ANNOT_ENUM) == 0)
		{
			prt::AnnotationArgumentType Type;

			if (AreArgumentsOfSameType(CEAnnotation, Type))
			{
				switch (Type)
				{
				case prt::AAT_FLOAT:
					InAttribute.SetAnnotation(ParseEnumAnnotation<UFloatEnumAnnotation, double>(CEAnnotation, Outer));
					break;
				case prt::AAT_STR:
					InAttribute.SetAnnotation(ParseEnumAnnotation<UStringEnumAnnotation, FString>(CEAnnotation, Outer));
					break;
				default:
					InAttribute.SetAnnotation({});
					break;
				}
			}
		}
		else if (std::wcscmp(Name, ANNOT_RANGE) == 0)
		{
			InAttribute.SetAnnotation(ParseRangeAnnotation(CEAnnotation, Outer));
		}
		else if (std::wcscmp(Name, ANNOT_DIR) == 0)
		{
			auto Annotation = NewObject<UFilesystemAnnotation>(Outer);
			Annotation->Mode = EFilesystemMode::Directory;
			InAttribute.SetAnnotation(Annotation);
		}
		else if (std::wcscmp(Name, ANNOT_FILE) == 0)
		{
			InAttribute.SetAnnotation(ParseFileAnnotation(CEAnnotation, Outer));
		}
		else if (std::wcscmp(Name, ANNOT_COLOR) == 0)
		{
			InAttribute.SetAnnotation(NewObject<UColorAnnotation>(Outer));
		}

		if (!std::wcscmp(Name, ANNOT_HIDDEN))
		{
			InAttribute.bHidden = true;
		}
		else if (!std::wcscmp(Name, ANNOT_ORDER))
		{
			InAttribute.Order = ParseOrder(CEAnnotation);
		}
		else if (!std::wcscmp(Name, ANNOT_GROUP))
		{
			ParseGroups(CEAnnotation, InAttribute);
		}
	}
}
} // namespace Vitruvio
