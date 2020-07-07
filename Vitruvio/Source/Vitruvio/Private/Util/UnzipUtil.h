// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "CoreUObject.h"

#include "Async/AsyncResult.h"
#include "Async/IAsyncProgress.h"

namespace Vitruvio
{
class FUnzipProgress : public IAsyncProgress
{
public:
	FUnzipProgress() = default;

	TOptional<float> GetCompletion() override
	{
		if (TotalFiles.IsSet())
		{
			return static_cast<float>(CurrentFile) / TotalFiles.GetValue();
		}
		return TOptional<float>();
	}

	FSimpleDelegate& OnProgressChanged() override { return ProgressDelegate; }

	FText GetStatusText() override { return FText::FromString("Unzipping"); }

	void SetTotal(int32 NumFiles) { TotalFiles = NumFiles; }

	void ReportProgress()
	{
		++CurrentFile;
		ProgressDelegate.ExecuteIfBound();
	}

private:
	int32 CurrentFile = 0;
	TOptional<int32> TotalFiles;
	FSimpleDelegate ProgressDelegate;
};

void Unzip(const FString& ZipPath, TSharedRef<TPromise<bool>, ESPMode::ThreadSafe> Promise, const TSharedPtr<FUnzipProgress>& UnzipProgress);
} // namespace Vitruvio