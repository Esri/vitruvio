/* Copyright 2024 Esri
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

#include "TextureDecoding.h"
#include "Engine/TextureDefines.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/Texture2D.h"
#include "Runtime/Engine/Public/TextureResource.h"
#include "UObject/Package.h"

#include <string>

namespace
{
struct FTextureSettings
{
	bool SRGB;
	TextureCompressionSettings Compression;
};

FTextureSettings GetTextureSettings(const FString& Key, EPixelFormat PixelFormat)
{
	if (Key == L"normalMap")
	{
		return {false, TC_Normalmap};
	}
	if (Key == L"roughnessMap" || Key == L"metallicMap")
	{
		return {false, TC_Masks};
	}
	bool IsGrayscale = PixelFormat == EPixelFormat::PF_G8 || PixelFormat == EPixelFormat::PF_G16 || EPixelFormat::PF_R32_FLOAT;
	return {!IsGrayscale, TC_Default};
}
} // namespace

namespace Vitruvio
{

FTextureMetadata ParseTextureMetadata(const prt::AttributeMap* TextureMetadata)
{
	Vitruvio::FTextureMetadata Result;
	Result.Width = TextureMetadata->getInt(L"width");
	Result.Height = TextureMetadata->getInt(L"height");
	Result.BytesPerBand = 0;
	Result.Bands = 0;

	FString Format(TextureMetadata->getString(L"format"));
	if (Format == TEXT("GREY8"))
	{
		Result.BytesPerBand = 1;
		Result.Bands = 1;
		Result.PixelFormat = EPRTPixelFormat::GREY8;
	}
	else if (Format == TEXT("GREY16"))
	{
		Result.BytesPerBand = 2;
		Result.Bands = 1;
		Result.PixelFormat = EPRTPixelFormat::GREY16;
	}
	else if (Format == TEXT("FLOAT32"))
	{
		Result.BytesPerBand = 4;
		Result.Bands = 1;
		Result.PixelFormat = EPRTPixelFormat::FLOAT32;
	}
	else if (Format == TEXT("RGB8"))
	{
		Result.BytesPerBand = 1;
		Result.Bands = 3;
		Result.PixelFormat = EPRTPixelFormat::RGB8;
	}
	else if (Format == TEXT("RGBA8"))
	{
		Result.BytesPerBand = 1;
		Result.Bands = 4;
		Result.PixelFormat = EPRTPixelFormat::RGBA8;
	}
	else
	{
		Result.PixelFormat = EPRTPixelFormat::Unknown;
	}

	return Result;
}

EPixelFormat GetUnrealPixelFormat(EPRTPixelFormat PRTPixelFormat)
{
	switch (PRTPixelFormat)
	{
	case EPRTPixelFormat::GREY8:
	case EPRTPixelFormat::RGB8:
	case EPRTPixelFormat::RGBA8:
	{
		return EPixelFormat::PF_B8G8R8A8;
	}
	case EPRTPixelFormat::FLOAT32:
	{
		return EPixelFormat::PF_FloatRGBA;
	}
	case EPRTPixelFormat::GREY16:
	{
		return EPixelFormat::PF_A16B16G16R16;
	}
	default:
	{
		return EPixelFormat::PF_Unknown;
	}
	}
}

FTextureData DecodeTexture(UObject* Outer, const FString& Key, const FString& Path, const FTextureMetadata& TextureMetadata,
						   std::unique_ptr<uint8_t[]> Buffer, size_t BufferSize)
{
	EPixelFormat UnrealPixelFormat = GetUnrealPixelFormat(TextureMetadata.PixelFormat);
	check(UnrealPixelFormat != EPixelFormat::PF_Unknown);

	const size_t BytesPerBand = FMath::Min<size_t>(2, TextureMetadata.BytesPerBand);
	const bool bIsColor = (TextureMetadata.Bands >= 3);

	size_t NewBufferSize = TextureMetadata.Width * TextureMetadata.Height * 4 * BytesPerBand;
	auto NewBuffer = std::make_unique<uint8_t[]>(NewBufferSize);

	for (int Y = 0; Y < TextureMetadata.Height; ++Y)
	{
		for (int X = 0; X < TextureMetadata.Width; ++X)
		{
			if (TextureMetadata.PixelFormat == EPRTPixelFormat::FLOAT32)
			{
				// Convert 32 bit grayscale float textures to 16 bit RGBA float textures
				const int OldOffset = ((TextureMetadata.Height - Y - 1) * TextureMetadata.Width + X) * TextureMetadata.Bands;
				const int NewOffset = (Y * TextureMetadata.Width + X);
				const float* FloatBuffer = reinterpret_cast<const float*>(Buffer.get());
				FFloat16Color* NewFloat16Buffer = reinterpret_cast<FFloat16Color*>(NewBuffer.get());
				const float Float32Value = FloatBuffer[OldOffset];
				const FFloat16 Float16Value(Float32Value);
				FFloat16Color Color;
				Color.R = Float16Value;
				Color.G = Float16Value;
				Color.B = Float16Value;
				Color.A = FFloat16(1.0f);
				NewFloat16Buffer[NewOffset] = Color;
			}
			else
			{
				// Workaround: Also convert grayscale images to rgba, since texture params don't automatically update their sample method
				const int OldOffset = ((TextureMetadata.Height - Y - 1) * TextureMetadata.Width + X) * TextureMetadata.Bands * BytesPerBand;
				const int NewOffset = (Y * TextureMetadata.Width + X) * 4 * BytesPerBand;
				for (int B = 0; B < BytesPerBand; ++B)
				{
					NewBuffer[NewOffset + 0 * BytesPerBand + B] = bIsColor ? Buffer[OldOffset + 2 + B] : Buffer[OldOffset + B];
					NewBuffer[NewOffset + 1 * BytesPerBand + B] = bIsColor ? Buffer[OldOffset + 1 + B] : Buffer[OldOffset + B];
					NewBuffer[NewOffset + 2 * BytesPerBand + B] = bIsColor ? Buffer[OldOffset + 0 + B] : Buffer[OldOffset + B];
					NewBuffer[NewOffset + 3 * BytesPerBand + B] = (TextureMetadata.Bands == 4) ? Buffer[OldOffset + 3 + B] : 0;
				}
			}
		}
	}

	const FTextureSettings Settings = GetTextureSettings(Key, UnrealPixelFormat);

	const FString TextureBaseName = TEXT("T_") + FPaths::GetBaseFilename(Path);
	const FName TextureName = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), *TextureBaseName);
	UTexture2D* NewTexture = NewObject<UTexture2D>(GetTransientPackage(), TextureName, RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
	NewTexture->CompressionSettings = Settings.Compression;
	NewTexture->SRGB = Settings.SRGB;

	FTexturePlatformData* PlatformData = new FTexturePlatformData();
	PlatformData = new FTexturePlatformData();
	PlatformData->SizeX = TextureMetadata.Width;
	PlatformData->SizeY = TextureMetadata.Height;
	PlatformData->PixelFormat = UnrealPixelFormat;

	// Allocate first mipmap and upload the pixel data
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	PlatformData->Mips.Add(Mip);
	Mip->SizeX = TextureMetadata.Width;
	Mip->SizeY = TextureMetadata.Height;
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	void* TextureData = Mip->BulkData.Realloc(CalculateImageBytes(TextureMetadata.Width, TextureMetadata.Height, 0, UnrealPixelFormat));
	FMemory::Memcpy(TextureData, NewBuffer.get(), NewBufferSize);
	Mip->BulkData.Unlock();

	NewTexture->SetPlatformData(PlatformData);

	NewTexture->UpdateResource();

	const auto TimeStamp = FPlatformFileManager::Get().GetPlatformFile().GetAccessTimeStamp(*Path);
	return FTextureData { NewTexture, static_cast<uint32>(TextureMetadata.Bands), TimeStamp };
}
} // namespace Vitruvio
