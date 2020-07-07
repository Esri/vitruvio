// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "UnzipUtil.h"

#include "ThirdParty/zlib/zlib-1.2.5/Src/contrib/minizip/unzip.h"

namespace Vitruvio
{
namespace Unzip
{

	bool Unzip(const FString& ZipPath)
	{
		static const int32 BUFFER_SIZE = 8096;
		static const int32 MAX_FILE_LENGTH = 512;

		const FString ZipFolder = FPaths::GetPath(ZipPath);
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		const unzFile ZipFile = unzOpen(TCHAR_TO_ANSI(*ZipPath));

		unz_global_info UnzGlobalInfo;
		if (unzGetGlobalInfo(ZipFile, &UnzGlobalInfo) != UNZ_OK)
		{
			unzClose(ZipFile);
			return false;
		}

		uint8 ReadBuffer[BUFFER_SIZE];
		FScopedSlowTask UnzipTask(static_cast<float>(UnzGlobalInfo.number_entry), FText::FromString("Unzipping PRT"));
		for (unsigned long FileIndex = 0; FileIndex < UnzGlobalInfo.number_entry; ++FileIndex)
		{
			UnzipTask.EnterProgressFrame(1);

			unz_file_info FileInfo;
			char Filename[MAX_FILE_LENGTH];
			if (unzGetCurrentFileInfo(ZipFile, &FileInfo, Filename, MAX_FILE_LENGTH, nullptr, 0, nullptr, 0) != UNZ_OK)
			{
				unzClose(ZipFile);
				return false;
			}

			const FString FullPath = FPaths::Combine(ZipFolder, ANSI_TO_TCHAR(Filename));
			PlatformFile.CreateDirectoryTree(*FPaths::GetPath(FullPath));

			IFileHandle* Handle = PlatformFile.OpenWrite(*FullPath, false, false);
			if (unzOpenCurrentFile(ZipFile) != UNZ_OK)
			{
				delete Handle;
				unzClose(ZipFile);
				return false;
			}

			int Read;
			while ((Read = unzReadCurrentFile(ZipFile, ReadBuffer, BUFFER_SIZE)) > 0)
			{
				Handle->Write(ReadBuffer, Read);
			}

			if (Read != UNZ_OK)
			{
				return false;
			}

			if (unzGoToNextFile(ZipFile) != UNZ_OK)
			{
				unzClose(ZipFile);
				return false;
			}

			unzCloseCurrentFile(ZipFile);
			Handle->Flush();
			delete Handle;
		}

		return true;
	}

} // namespace Unzip
} // namespace Vitruvio