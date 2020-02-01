#include "UnrealGeometryEncoderModule.h"
#include "Codec/Encoder/UnrealResolveMapProvider.h"
#include "Util/UnrealPRTUtils.h"

#include "prt/API.h"
#include "prt/MemoryOutputCallbacks.h"

#include "Core.h"
#include "Interfaces/IPluginManager.h"
#include "MeshDescription.h"
#include "Modules/ModuleManager.h"
#include "StaticMeshAttributes.h"
#include "UObject/UObjectBaseUtility.h"
#include "UnrealCallbacks.h"
#include "prtx/EncoderInfoBuilder.h"

#define LOCTEXT_NAMESPACE "UnrealGeometryEncoderModule"

DEFINE_LOG_CATEGORY(LogUnrealPrt);

namespace
{
	constexpr const wchar_t* ENC_ID_ATTR_EVAL = L"com.esri.prt.core.AttributeEvalEncoder";

	void SetInitialShapeGeometry(const InitialShapeBuilderUPtr& InitialShapeBuilder, const UStaticMesh* InitialShape)
	{
		const FMeshDescription* MeshDescription = InitialShape->GetMeshDescription(0);
		const FStaticMeshConstAttributes Attributes(*MeshDescription);
		const auto& VertexPositions = Attributes.GetVertexPositions();

		std::vector<double> vertexCoords;
		for (const FVertexID VertexID : MeshDescription->Vertices().GetElementIDs())
		{
			vertexCoords.push_back(VertexPositions[VertexID][0]);
			vertexCoords.push_back(VertexPositions[VertexID][1]);
			vertexCoords.push_back(VertexPositions[VertexID][2]);
		}

		std::vector<uint32_t> indices;
		std::vector<uint32_t> faceCounts;
		const auto& Triangles = MeshDescription->Triangles();
		for (const FPolygonID PolygonID : MeshDescription->Polygons().GetElementIDs())
		{
			for (const FTriangleID TriangleID : MeshDescription->GetPolygonTriangleIDs(PolygonID))
			{
				for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
				{
					const FVertexInstanceID VertexInstanceID = MeshDescription->GetTriangleVertexInstance(TriangleID, CornerIndex);
					FVertexID VertexID = MeshDescription->GetVertexInstanceVertex(VertexInstanceID);
					indices.push_back(static_cast<uint32_t>(VertexID.GetValue()));
				}
				faceCounts.push_back(3);
			}
		}

		const prt::Status SetGeometryStatus =
			InitialShapeBuilder->setGeometry(vertexCoords.data(), vertexCoords.size(), indices.data(), indices.size(), faceCounts.data(), faceCounts.size());

		if (SetGeometryStatus != prt::STATUS_OK)
		{
			UE_LOG(LogUnrealPrt, Error, TEXT("InitialShapeBuilder setGeometry failed status = %hs"), prt::getStatusDescription(SetGeometryStatus))
		}
	}

	AttributeMapUPtr GetDefaultAttributeValues(const std::wstring& RuleFile, const std::wstring& StartRule, const ResolveMapSPtr& ResolveMapPtr, const UStaticMesh* InitialShape)
	{

		AttributeMapBuilderUPtr UnrealCallbacksAttributeBuilder(prt::AttributeMapBuilder::create());
		UnrealCallbacks UnrealCallbacks(UnrealCallbacksAttributeBuilder);

		InitialShapeBuilderUPtr InitialShapeBuilder(prt::InitialShapeBuilder::create());

		SetInitialShapeGeometry(InitialShapeBuilder, InitialShape);

		// TODO calculate random seed
		const int32_t RandomSeed = 0;
		const AttributeMapUPtr EmptyAttributes(AttributeMapBuilderUPtr(prt::AttributeMapBuilder::create())->createAttributeMap());
		InitialShapeBuilder->setAttributes(RuleFile.c_str(), StartRule.c_str(), RandomSeed, L"", EmptyAttributes.get(), ResolveMapPtr.get());

		const InitialShapeUPtr Shape(InitialShapeBuilder->createInitialShapeAndReset());
		const InitialShapeNOPtrVector InitialShapes = {Shape.get()};

		const std::vector<const wchar_t*> EncoderIds = {ENC_ID_ATTR_EVAL};
		const AttributeMapUPtr AttributeEncodeOptions = prtu::createValidatedOptions(ENC_ID_ATTR_EVAL);
		const AttributeMapNOPtrVector EncoderOptions = {AttributeEncodeOptions.get()};

		prt::generate(InitialShapes.data(), InitialShapes.size(), nullptr, EncoderIds.data(), EncoderIds.size(), EncoderOptions.data(), &UnrealCallbacks, nullptr, nullptr);

		return AttributeMapUPtr(UnrealCallbacksAttributeBuilder->createAttributeMap());
	}

	AttributeMapUPtr CreateAttributeMap(const TMap<FString, URuleAttribute*>& Attributes)
	{
		AttributeMapBuilderUPtr AttributeMapBuilder(prt::AttributeMapBuilder::create());

		for (const TPair<FString, URuleAttribute*>& AttributeEntry : Attributes)
		{
			const URuleAttribute* Attribute = AttributeEntry.Value;

			// TODO implement all types (see: https://github.com/Esri/serlio/blob/b293b660034225371101ef1e9a3d9cfafb3c5382/src/serlio/prtModifier/PRTModifierAction.cpp#L144)
			if (const UFloatAttribute* FloatAttribute = Cast<UFloatAttribute>(Attribute))
			{
				AttributeMapBuilder->setFloat(*Attribute->Name, FloatAttribute->Value);
			}
			else if (const UStringAttribute* StringAttribute = Cast<UStringAttribute>(Attribute))
			{
				AttributeMapBuilder->setString(*Attribute->Name, *StringAttribute->Value);
			}
			else if (const UBoolAttribute* BoolAttribute = Cast<UBoolAttribute>(Attribute))
			{
				AttributeMapBuilder->setBool(*Attribute->Name, BoolAttribute->Value);
			}
		}

		return AttributeMapUPtr(AttributeMapBuilder->createAttributeMap(), PRTDestroyer());
	}

} // namespace

void UnrealGeometryEncoderModule::StartupModule()
{
	const FString BaseDir = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin("UnrealGeometryEncoder")->GetBaseDir());
	const FString BinariesPath = FPaths::Combine(*BaseDir, L"Binaries", L"Win64");
	const FString ExtensionsFolder = FPaths::Combine(*BaseDir, L"Binaries", L"Win64", L"lib");
	const FString PrtLibPath = FPaths::Combine(*BinariesPath, L"com.esri.prt.core.dll");

	PrtDllHandle = FPlatformProcess::GetDllHandle(*PrtLibPath);

	TArray<wchar_t*> PRTPluginsPaths;
	PRTPluginsPaths.Add(const_cast<wchar_t*>(*ExtensionsFolder));

	LogHandler = new UnrealLogHandler;
	prt::addLogHandler(LogHandler);

	prt::Status Status;
	PrtLibrary = prt::init(PRTPluginsPaths.GetData(), PRTPluginsPaths.Num(), prt::LogLevel::LOG_TRACE, &Status);
	Initialized = Status == prt::STATUS_OK;
}

void UnrealGeometryEncoderModule::ShutdownModule()
{
	if (PrtDllHandle)
	{
		FPlatformProcess::FreeDllHandle(PrtDllHandle);
		PrtDllHandle = nullptr;
	}
	if (PrtLibrary)
	{
		PrtLibrary->destroy();
	}

	delete LogHandler;
}

ResolveMapSPtr UnrealGeometryEncoderModule::GetResolveMap(const std::wstring& Uri) const
{
	const auto ResultIter = ResolveMapCache.find(Uri);
	if (ResultIter != ResolveMapCache.end())
	{
		return ResultIter->second;
	}

	// TODO also add timestamp to cache entry (see: https://github.com/Esri/serlio/blob/master/src/serlio/util/ResolveMapCache.cpp)
	ResolveMapSPtr ResolveMap(prt::createResolveMap(Uri.c_str()), PRTDestroyer());
	ResolveMapCache.emplace(Uri, ResolveMap);
	return ResolveMap;
}

FGenerateResult UnrealGeometryEncoderModule::Generate(const UStaticMesh* InitialShape, URulePackage* RulePackage, const TMap<FString, URuleAttribute*>& Attributes) const
{
	check(InitialShape);
	check(RulePackage);
	
	if (!Initialized)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("prt not initialized"))
		return {};
	}

	const InitialShapeBuilderUPtr InitialShapeBuilder(prt::InitialShapeBuilder::create());
	SetInitialShapeGeometry(InitialShapeBuilder, InitialShape);

	const FString PathName = RulePackage->GetPathName();
	const std::wstring PathUri = UnrealResolveMapProvider::SCHEME_UNREAL + L":" + *PathName;
	const ResolveMapSPtr ResolveMap = GetResolveMap(PathUri);

	const std::wstring RuleFile = prtu::getRuleFileEntry(ResolveMap);
	const wchar_t* RuleFileUri = ResolveMap->getString(RuleFile.c_str());

	const RuleFileInfoUPtr StartRuleInfo(prt::createRuleFileInfo(RuleFileUri));
	const std::wstring StartRule = prtu::detectStartRule(StartRuleInfo);

	// TODO calculate random seed
	const int32_t RandomSeed = 0;
	const AttributeMapUPtr AttributeMap = CreateAttributeMap(Attributes);
	InitialShapeBuilder->setAttributes(RuleFile.c_str(), StartRule.c_str(), RandomSeed, L"", AttributeMap.get(), ResolveMap.get());

	AttributeMapBuilderUPtr AttributeMapBuilder(prt::AttributeMapBuilder::create());
	const std::unique_ptr<UnrealCallbacks> OutputHandler(new UnrealCallbacks(AttributeMapBuilder));

	const InitialShapeUPtr Shape(InitialShapeBuilder->createInitialShapeAndReset());

	const std::vector<const wchar_t*> EncoderIds = {ENCODER_ID_UnrealGeometry};
	const AttributeMapUPtr UnrealEncoderOptions(prtu::createValidatedOptions(ENCODER_ID_UnrealGeometry));
	const AttributeMapNOPtrVector EncoderOptions = {UnrealEncoderOptions.get()};

	InitialShapeNOPtrVector Shapes = {Shape.get()};

	const prt::Status GenerateStatus =
		prt::generate(Shapes.data(), Shapes.size(), nullptr, EncoderIds.data(), EncoderIds.size(), EncoderOptions.data(), OutputHandler.get(), nullptr, nullptr);

	if (GenerateStatus != prt::STATUS_OK)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("prt generate failed: %hs"), prt::getStatusDescription(GenerateStatus))
	}

	TArray<UInstancedStaticMeshComponent*> InstanceComponents;
	OutputHandler->getInstances().GenerateValueArray(InstanceComponents);
	return { OutputHandler->getShapeMesh(), InstanceComponents };
}

void UnrealGeometryEncoderModule::LoadDefaultRuleAttributes(const UStaticMesh* InitialShape, URulePackage* RulePackage, TMap<FString, URuleAttribute*>& OutAttributes) const
{
	check(InitialShape);
	check(RulePackage);
	
	const FString PathName = RulePackage->GetPathName();
	const std::wstring PathUri = UnrealResolveMapProvider::SCHEME_UNREAL + L":" + *PathName;
	const ResolveMapSPtr ResolveMap = GetResolveMap(PathUri);

	const std::wstring RuleFile = prtu::getRuleFileEntry(ResolveMap);
	const wchar_t* RuleFileUri = ResolveMap->getString(RuleFile.c_str());

	const RuleFileInfoUPtr StartRuleInfo(prt::createRuleFileInfo(RuleFileUri));
	const std::wstring StartRule = prtu::detectStartRule(StartRuleInfo);

	prt::Status InfoStatus;
	const RuleFileInfoUPtr RuleInfo(prt::createRuleFileInfo(RuleFileUri, nullptr, &InfoStatus));
	if (!RuleInfo || InfoStatus != prt::STATUS_OK)
	{
		UE_LOG(LogUnrealPrt, Error, TEXT("could not get rule file info from rule file %s"), RuleFileUri)
		return;
	}

	const AttributeMapUPtr DefaultAttributeMap(GetDefaultAttributeValues(RuleFile.c_str(), StartRule.c_str(), ResolveMap, InitialShape));

	for (size_t AttributeIndex = 0; AttributeIndex < RuleInfo->getNumAttributes(); AttributeIndex++)
	{
		const prt::RuleFileInfo::Entry* AttrInfo = RuleInfo->getAttribute(AttributeIndex);
		const std::wstring Name(AttrInfo->getName());

		if (OutAttributes.Contains(Name.c_str()))
		{
			continue;
		}

		URuleAttribute* Attribute = nullptr;

		switch (AttrInfo->getReturnType())
		{
		// TODO implement all types as well as annotation parsing (see:
		// https://github.com/Esri/serlio/blob/b293b660034225371101ef1e9a3d9cfafb3c5382/src/serlio/prtModifier/PRTModifierAction.cpp#L358)
		case prt::AAT_BOOL:
		{
			UBoolAttribute* BoolAttribute = NewObject<UBoolAttribute>();
			BoolAttribute->Value = DefaultAttributeMap->getBool(Name.c_str());
			Attribute = BoolAttribute;
			break;
		}
		case prt::AAT_FLOAT:
		{
			UFloatAttribute* FloatAttribute = NewObject<UFloatAttribute>();
			FloatAttribute->Value = DefaultAttributeMap->getFloat(Name.c_str());
			Attribute = FloatAttribute;
			break;
		}
		case prt::AAT_STR:
		{
			UStringAttribute* StringAttribute = NewObject<UStringAttribute>();
			StringAttribute->Value = DefaultAttributeMap->getString(Name.c_str());
			Attribute = StringAttribute;
			break;
		}
		case prt::AAT_INT:
			break;
		case prt::AAT_UNKNOWN:
			break;
		case prt::AAT_VOID:
			break;
		case prt::AAT_BOOL_ARRAY:
			break;
		case prt::AAT_FLOAT_ARRAY:
			break;
		case prt::AAT_STR_ARRAY:
			break;
		default:;
		}

		if (Attribute)
		{
			FString AttributeName = Name.c_str();
			Attribute->Name = AttributeName;
			OutAttributes.Add(AttributeName, Attribute);
		}
	}
}

#undef LOCTEXT_NAMESPACE

// We don't want to overwrite new/delete for the module but setting FORCE_ANSI_ALLOCATOR to 1 does not work so we change
// macros defined in ModuleBoilerplate.h to just not overwrite them.

// in DLL builds, these are done per-module, otherwise we just need one in the application
// visual studio cannot find cross dll data for visualizers, so these provide access
#define PER_MODULE_BOILERPLATE_UNREAL UE4_VISUALIZERS_HELPERS

#define IMPLEMENT_MODULE_UNREAL(ModuleImplClass, ModuleName)                                                                                                                       \
                                                                                                                                                                                   \
	/**/                                                                                                                                                                           \
	/* InitializeModule function, called by module manager after this module's DLL has been loaded */                                                                              \
	/**/                                                                                                                                                                           \
	/* @return	Returns an instance of this module */                                                                                                                               \
	/**/                                                                                                                                                                           \
	extern "C" DLLEXPORT IModuleInterface* InitializeModule()                                                                                                                      \
	{                                                                                                                                                                              \
		return new ModuleImplClass();                                                                                                                                              \
	}                                                                                                                                                                              \
	/* Forced reference to this function is added by the linker to check that each module uses IMPLEMENT_MODULE */                                                                 \
	extern "C" void IMPLEMENT_MODULE_##ModuleName()                                                                                                                                \
	{                                                                                                                                                                              \
	}                                                                                                                                                                              \
	PER_MODULE_BOILERPLATE_UNREAL                                                                                                                                                  \
	PER_MODULE_BOILERPLATE_ANYLINK(ModuleImplClass, ModuleName)

IMPLEMENT_MODULE_UNREAL(UnrealGeometryEncoderModule, UnrealGeometryEncoder)