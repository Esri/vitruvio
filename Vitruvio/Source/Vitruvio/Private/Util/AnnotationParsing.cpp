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

	prt::AnnotationArgumentType GetEnumAnnotationType(const prt::Annotation* Annotation)
	{
		prt::AnnotationArgumentType Type = prt::AAT_UNKNOWN;
		for (size_t ArgumentIndex = 0; ArgumentIndex < Annotation->getNumArguments(); ArgumentIndex++) {
			if (Type != prt::AAT_UNKNOWN &&  Type != Annotation->getArgument(ArgumentIndex)->getType())
			{
				return prt::AAT_UNKNOWN;
			}
			Type = Annotation->getArgument(ArgumentIndex)->getType();
		}
		return Type;
	}

	void ParseValue(const prt::AnnotationArgument* Argument, double& Result)
	{
		Result = Argument->getFloat();
	}

	void ParseValue(const prt::AnnotationArgument* Argument, FString& Result)
	{
		Result = FString(Argument->getStr());
	}

	template <typename T>
	TSharedPtr<EnumAnnotation<T>> ParseEnumAnnotation(const prt::Annotation* Annotation)
	{
		prt::AnnotationArgumentType Type = prt::AAT_UNKNOWN;

		auto Result = MakeShared<EnumAnnotation<T>>();
		
		for (size_t ArgumentIndex = 0; ArgumentIndex < Annotation->getNumArguments(); ArgumentIndex++) {
			const wchar_t* Key = Annotation->getArgument(ArgumentIndex)->getKey();
			if (std::wcscmp(Key, NULL_KEY) != 0) {
				if (std::wcscmp(Key, RESTRICTED_KEY) == 0) {
					Result->Restricted = Annotation->getArgument(ArgumentIndex)->getBool();
				}
				continue;
			}

			T Val;
			ParseValue(Annotation->getArgument(ArgumentIndex), Val);
			Result->Values.Add(Val);
		}
		return Result;
	}

	TSharedPtr<RangeAnnotation> ParseRangeAnnotation(const prt::Annotation* Annotation)
	{
		auto Result = MakeShared<RangeAnnotation>();
		Result->StepSize = 0.1;
		
		for (int ArgIndex = 0; ArgIndex < Annotation->getNumArguments(); ArgIndex++)
		{
			const prt::AnnotationArgument* Argument = Annotation->getArgument(ArgIndex);
			const wchar_t* Key = Argument->getKey();
			if (std::wcscmp(Key, MIN_KEY) == 0)
			{
				Result->Min = Argument->getFloat();
			}
			else if (std::wcscmp(Key, MAX_KEY) == 0)
			{
				Result->Max = Argument->getFloat();
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

	TSharedPtr<FilesystemAnnotation> ParseFileAnnotation(const prt::Annotation* Annotation)
	{
		FString Extensions;
		for (size_t ArgumentIndex = 0; ArgumentIndex < Annotation->getNumArguments(); ArgumentIndex++) {
			if (Annotation->getArgument(ArgumentIndex)->getType() == prt::AAT_STR) {
				Extensions += Annotation->getArgument(ArgumentIndex)->getStr();
				Extensions += L" (*.";
				Extensions += Annotation->getArgument(ArgumentIndex)->getStr();
				Extensions += L");";
			}
		}
		Extensions += L"All Files (*.*)";
		
		auto Result = MakeShared<FilesystemAnnotation>();
		Result->Mode = File;
		Result->Extensions = Extensions;
		return Result;
	}

	int ParseOrder(const prt::Annotation* Annotation)
	{
		return 0;
	}

	int ParseGroupOrder(const prt::Annotation* Annotation)
	{
		return 0;
	}
	
	FAttributeGroups ParseGroups(const prt::Annotation* Annotation)
	{
		return {};
	}
}

void ParseAttributeAnnotations(const prt::RuleFileInfo::Entry* AttributeInfo, URuleAttribute& InAttribute)
{
	for (size_t AnnotationIndex = 0; AnnotationIndex < AttributeInfo->getNumAnnotations(); ++AnnotationIndex)
	{
		const prt::Annotation* CEAnnotation = AttributeInfo->getAnnotation(AnnotationIndex);
		
		const wchar_t* Name = CEAnnotation->getName();
		if (std::wcscmp(Name, ANNOT_ENUM) == 0)
		{
			switch(GetEnumAnnotationType(CEAnnotation))
			{
			case prt::AAT_FLOAT:
				InAttribute.SetAnnotation(ParseEnumAnnotation<double>(CEAnnotation));
				break;
			case prt::AAT_STR:
				InAttribute.SetAnnotation(ParseEnumAnnotation<FString>(CEAnnotation));
				break;
			 default:
				InAttribute.SetAnnotation({});
				break;
			}
			
		}
		else if (std::wcscmp(Name, ANNOT_RANGE) == 0)
		{
			InAttribute.SetAnnotation(ParseRangeAnnotation(CEAnnotation));
		}
		else if (std::wcscmp(Name, ANNOT_DIR) == 0)
		{
			auto Annotation = MakeShared<FilesystemAnnotation>();
			Annotation->Mode = Directory;
			InAttribute.SetAnnotation(Annotation);
		}
		else if (std::wcscmp(Name, ANNOT_FILE) == 0)
		{
			InAttribute.SetAnnotation(ParseFileAnnotation(CEAnnotation));
		}

		if (!std::wcscmp(Name, ANNOT_HIDDEN))
		{
			InAttribute.Hidden = true;
		}
		else if (!std::wcscmp(Name, ANNOT_ORDER))
		{
			InAttribute.Order = ParseOrder(CEAnnotation);
		}
		else if (!std::wcscmp(Name, ANNOT_GROUP))
		{
			InAttribute.Groups = ParseGroups(CEAnnotation);
			InAttribute.GroupOrder = ParseGroupOrder(CEAnnotation);
		}
	}
}