#include "UnrealResolveMapProvider.h"

#include "prt/API.h"
#include "prt/ResolveMap.h"
#include "prtx/ExtensionManager.h"

#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "RulePackage.h"
#include "PRTUtils.h"

const std::wstring UnrealResolveMapProvider::ID = L"com.esri.prt.adaptors.UnrealResolveMapProvider";
const std::wstring UnrealResolveMapProvider::NAME = L"Unreal ResolveMapProvider";
const std::wstring UnrealResolveMapProvider::DESCRIPTION = L"Resolves URIs inside Unreal Asset RPKs.";
const std::wstring UnrealResolveMapProvider::SCHEME_UNREAL = L"Unreal";

prt::ResolveMap const* UnrealResolveMapProvider::createResolveMap(prtx::URIPtr uri) const
{
	const std::wstring Uri = uri->wstring();
	const FString FullUri = Uri.c_str();

	FString Scheme;
	FString UriPath;
	FullUri.Split(L":", &Scheme, &UriPath);

	const FString PackageName = L"RulePackage'" + UriPath + L"'";
	UObject* RulePackageObject = StaticLoadObject(URulePackage::StaticClass(), nullptr, *PackageName);

	if (URulePackage* RulePackage = Cast<URulePackage>(RulePackageObject))
	{
		// Create rpk on disk for PRT
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		FString TempDir(prtu::temp_directory_path().c_str());
		const FString RpkFolder = FPaths::Combine(TempDir, L"PRT", L"UnrealGeometryEncoder", FPaths::GetPath(UriPath.Mid(1)));
		const FString RpkFile = FPaths::GetBaseFilename(UriPath, true) + ".rpk";
		const FString RpkPath = FPaths::Combine(RpkFolder, RpkFile);
		PlatformFile.CreateDirectoryTree(*RpkFolder);
		IFileHandle* RpkHandle = PlatformFile.OpenWrite(*RpkPath);
		if (RpkHandle)
		{
			// Write file to disk
			RpkHandle->Write(RulePackage->Data.GetData(), RulePackage->Data.Num());
			RpkHandle->Flush();
			delete RpkHandle;

			// Create rpk
			const std::wstring AbsoluteRpkPath(*FPaths::ConvertRelativePathToFull(RpkPath));
			const std::wstring AbsoluteRpkFolder(
				*FPaths::Combine(FPaths::GetPath(FPaths::ConvertRelativePathToFull(RpkPath)), FPaths::GetBaseFilename(UriPath, true) + L"_Unpacked"));
			const std::wstring RpkFileUri = prtu::toFileURI(AbsoluteRpkPath);

			prt::Status Status;
			prt::ResolveMap const* ResolveMap = prt::createResolveMap(RpkFileUri.c_str(), AbsoluteRpkFolder.c_str(), &Status);

			return ResolveMap;
		}
	}

	return nullptr;
}

UnrealResolveMapProviderFactory::~UnrealResolveMapProviderFactory()
{
	// Cleanup temp RPK directory
	FString TempDir(prtu::temp_directory_path().c_str());
	const FString RpkUnpackFolder = FPaths::Combine(TempDir, L"PRT", L"UnrealGeometryEncoder");
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.DeleteDirectoryRecursively(*RpkUnpackFolder);
}

bool UnrealResolveMapProviderFactory::canHandleURI(prtx::URIPtr uri) const
{
	return uri->getScheme() == UnrealResolveMapProvider::SCHEME_UNREAL;
}
