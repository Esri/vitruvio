// Copyright 2019 - 2020 Esri. All Rights Reserved.

#include "UnrealResolveMapProvider.h"

#include "PRTUtils.h"
#include "RulePackage.h"

#pragma warning(push)
#pragma warning(disable : 4263 4264)
#include "prt/API.h"
#include "prt/ResolveMap.h"
#include "prtx/ExtensionManager.h"
#pragma warning(pop)

#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

const std::wstring UnrealResolveMapProvider::ID = L"com.esri.prt.adaptors.UnrealResolveMapProvider";
const std::wstring UnrealResolveMapProvider::NAME = L"Unreal ResolveMapProvider";
const std::wstring UnrealResolveMapProvider::DESCRIPTION = L"Resolves URIs inside Unreal Asset RPKs.";
const std::wstring UnrealResolveMapProvider::SCHEME_UNREAL = L"Unreal";

prt::ResolveMap const* UnrealResolveMapProvider::createResolveMap(prtx::URIPtr uri) const
{
	const FString FullUri = WCHAR_TO_TCHAR(uri->wstring().c_str());

	FString Scheme;
	FString UriPath;
	FullUri.Split(TEXT(":"), &Scheme, &UriPath);

	const FString PackageName = TEXT("RulePackage'") + UriPath + TEXT("'");
	UObject* RulePackageObject = StaticLoadObject(URulePackage::StaticClass(), nullptr, *PackageName);

	if (URulePackage* RulePackage = Cast<URulePackage>(RulePackageObject))
	{
		// Create rpk on disk for PRT
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		FString TempDir(WCHAR_TO_TCHAR(prtu::temp_directory_path().c_str()));
		const FString RpkFolder = FPaths::Combine(TempDir, TEXT("PRT"), TEXT("UnrealGeometryEncoder"), FPaths::GetPath(UriPath.Mid(1)));
		const FString RpkFile = FPaths::GetBaseFilename(UriPath, true) + TEXT(".rpk");
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
			const std::wstring AbsoluteRpkPath(TCHAR_TO_WCHAR(*FPaths::ConvertRelativePathToFull(RpkPath)));
			const std::wstring AbsoluteRpkFolder(TCHAR_TO_WCHAR(*FPaths::Combine(FPaths::GetPath(FPaths::ConvertRelativePathToFull(RpkPath)),
																				 FPaths::GetBaseFilename(UriPath, true) + TEXT("_Unpacked"))));
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
	FString TempDir(WCHAR_TO_TCHAR(prtu::temp_directory_path().c_str()));
	const FString RpkUnpackFolder = FPaths::Combine(TempDir, TEXT("PRT"), TEXT("UnrealGeometryEncoder"));
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.DeleteDirectoryRecursively(*RpkUnpackFolder);
}

bool UnrealResolveMapProviderFactory::canHandleURI(prtx::URIPtr uri) const
{
	return uri->getScheme() == UnrealResolveMapProvider::SCHEME_UNREAL;
}
