// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "UnzipUtil.h"

#include "ThirdParty/zlib/zlib-1.2.5/Src/contrib/minizip/unzip.h"

namespace Vitruvio
{
namespace Unzip
{

	TAsyncResult<bool> Unzip(const FString& ZipPath, const TSharedPtr<FUnzipProgress>& UnzipProgress)
	{
		static const int32 BUFFER_SIZE = 8096;
		static const int32 MAX_FILE_LENGTH = 512;

		const FString ZipFolder = FPaths::GetPath(ZipPath);
		const unzFile ZipFile = unzOpen(TCHAR_TO_ANSI(*ZipPath));

		unz_global_info UnzGlobalInfo;
		if (unzGetGlobalInfo(ZipFile, &UnzGlobalInfo) != UNZ_OK)
		{
			unzClose(ZipFile);
			return false;
		}

		const TSharedRef<TPromise<bool>, ESPMode::ThreadSafe> Promise = MakeShareable(new TPromise<bool>());

		AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [UnzGlobalInfo, ZipFile, Promise, ZipFolder, UnzipProgress]() {
			uint8 ReadBuffer[BUFFER_SIZE];
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

			UnzipProgress->SetTotal(UnzGlobalInfo.number_entry);

			for (unsigned long FileIndex = 0; FileIndex < UnzGlobalInfo.number_entry; ++FileIndex)
			{
				UnzipProgress->ReportProgress();

				unz_file_info FileInfo;
				char Filename[MAX_FILE_LENGTH];
				if (unzGetCurrentFileInfo(ZipFile, &FileInfo, Filename, MAX_FILE_LENGTH, nullptr, 0, nullptr, 0) != UNZ_OK)
				{
					unzClose(ZipFile);
					Promise->SetValue(false);
					return;
				}

				const FString FullPath = FPaths::Combine(ZipFolder, ANSI_TO_TCHAR(Filename));
				PlatformFile.CreateDirectoryTree(*FPaths::GetPath(FullPath));

				IFileHandle* Handle = PlatformFile.OpenWrite(*FullPath, false, false);
				if (unzOpenCurrentFile(ZipFile) != UNZ_OK)
				{
					delete Handle;
					unzClose(ZipFile);
					Promise->SetValue(false);
					return;
				}

				int Read;
				while ((Read = unzReadCurrentFile(ZipFile, ReadBuffer, BUFFER_SIZE)) > 0)
				{
					Handle->Write(ReadBuffer, Read);
				}

				if (Read != UNZ_OK)
				{
					Promise->SetValue(false);
					return;
				}

				if (unzGoToNextFile(ZipFile) != UNZ_OK)
				{
					unzClose(ZipFile);
					Promise->SetValue(false);
					return;
				}

				unzCloseCurrentFile(ZipFile);
				Handle->Flush();
				delete Handle;
			}
			Promise->SetValue(true);
		});

		return TAsyncResult<bool>(Promise->GetFuture(), UnzipProgress, nullptr);
	}

} // namespace Unzip
} // namespace Vitruvio