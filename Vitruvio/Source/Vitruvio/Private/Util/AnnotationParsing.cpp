#include "AnnotationParsing.h"

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
	
	UAttributeAnnotation* ParseEnumAnnotation(const prt::Annotation* Annotation)
	{
		return NewObject<UEnumAnnotation>();
	}

	UAttributeAnnotation* ParseRangeAnnotation(const prt::Annotation* Annotation)
	{
		return NewObject<URangeAnnotation>();
	}

	UAttributeAnnotation* ParseColorAnnotation(const prt::Annotation* Annotation)
	{
		return NewObject<UColorAnnotation>();
	}

	UAttributeAnnotation* ParseDirAnnotation(const prt::Annotation* Annotation)
	{
		return NewObject<UFilesystemAnnotation>();
	}

	UAttributeAnnotation* ParseFileAnnotation(const prt::Annotation* Annotation)
	{
		return NewObject<UFilesystemAnnotation>();
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

UAttributeMetadata* ParseAttributeMetadata(const prt::RuleFileInfo::Entry* AttributeInfo)
{
	UAttributeMetadata* Metadata = NewObject<UAttributeMetadata>();
	
	for (size_t AnnotationIndex = 0; AnnotationIndex < AttributeInfo->getNumAnnotations(); ++AnnotationIndex)
	{
		const prt::Annotation* CEAnnotation = AttributeInfo->getAnnotation(AnnotationIndex);
		
		const wchar_t* Name = CEAnnotation->getName();
		if (std::wcscmp(Name, ANNOT_ENUM) == 0)
		{
			Metadata->Annotation = ParseEnumAnnotation(CEAnnotation);
		}
		else if (std::wcscmp(Name, ANNOT_RANGE) == 0)
		{
			Metadata->Annotation = ParseRangeAnnotation(CEAnnotation);
		}
		else if (std::wcscmp(Name, ANNOT_COLOR) == 0)
		{
			Metadata->Annotation = ParseColorAnnotation(CEAnnotation);
		}
		else if (std::wcscmp(Name, ANNOT_DIR) == 0) {
			Metadata->Annotation = ParseDirAnnotation(CEAnnotation);
		}
		else if (std::wcscmp(Name, ANNOT_FILE) == 0)
		{
			Metadata->Annotation = ParseFileAnnotation(CEAnnotation);
		}

		if (!std::wcscmp(Name, ANNOT_HIDDEN))
		{
			Metadata->Hidden = true;
		}
		else if (!std::wcscmp(Name, ANNOT_ORDER))
		{
			Metadata->Order = ParseOrder(CEAnnotation);
		}
		else if (!std::wcscmp(Name, ANNOT_GROUP))
		{
			Metadata->Groups = ParseGroups(CEAnnotation);
			Metadata->GroupOrder = ParseGroupOrder(CEAnnotation);
		}
	}

	return Metadata;
}