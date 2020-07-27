// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "MaterialConversion.h"

#include "PRTTypes.h"
#include "RuleAttributes.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImageCore/Public/ImageCore.h"
#include "VitruvioTypes.h"

#include <map>

DEFINE_LOG_CATEGORY(LogMaterialConversion);

namespace
{

constexpr double BLACK_COLOR_THRESHOLD = 0.02;
constexpr double WHITE_COLOR_THRESHOLD = 1.0 - BLACK_COLOR_THRESHOLD;
constexpr double OPACITY_THRESHOLD = 0.98;

struct FTextureSettings
{
	bool SRGB;
	TextureCompressionSettings Compression;
};

template <typename T, typename F>
void CountOpacityMapPixels(const T* SrcColors, int32 SizeX, int32 SizeY, uint32& BlackPixels, uint32& WhitePixels, F Accessor)
{
	BlackPixels = 0;
	WhitePixels = 0;

	const T* LastColor = SrcColors + (SizeX * SizeY);
	while (SrcColors < LastColor)
	{
		const float Value = Accessor(SrcColors);

		if (Value < BLACK_COLOR_THRESHOLD)
		{
			BlackPixels++;
		}
		else if (Value > WHITE_COLOR_THRESHOLD)
		{
			WhitePixels++;
		}
		++SrcColors;
	}
}

void CountOpacityMapPixels(const FColor* SrcColors, bool UseAlphaChannel, int32 SizeX, int32 SizeY, uint32& BlackPixels, uint32& WhitePixels)
{
	return CountOpacityMapPixels(SrcColors, SizeX, SizeY, BlackPixels, WhitePixels,
								 [UseAlphaChannel](const FColor* Color) { return static_cast<float>(UseAlphaChannel ? Color->A : Color->R) / 0xFF; });
}

void CountOpacityMapPixels(const uint8* SrcColors, int32 SizeX, int32 SizeY, uint32& BlackPixels, uint32& WhitePixels)
{
	return CountOpacityMapPixels(SrcColors, SizeX, SizeY, BlackPixels, WhitePixels,
								 [](const uint8* Color) { return static_cast<float>(*Color) / 0xFF; });
}

void CountOpacityMapPixels(const uint16* SrcColors, int32 SizeX, int32 SizeY, uint32& BlackPixels, uint32& WhitePixels)
{
	return CountOpacityMapPixels(SrcColors, SizeX, SizeY, BlackPixels, WhitePixels,
								 [](const uint16* Color) { return static_cast<float>(*Color) / 0xFFFF; });
}

bool HasAlpha(const FColor* SrcColors, int32 SizeX, int32 SizeY)
{
	const FColor* LastColor = SrcColors + (SizeX * SizeY);
	while (SrcColors < LastColor)
	{
		if ((static_cast<float>(SrcColors->A) / 0xFF) < (1.0f - SMALL_NUMBER))
		{
			return true;
		}
		++SrcColors;
	}
	return false;
}

ERGBFormat GetRequestedFormat(ERGBFormat Format)
{
	switch (Format)
	{
	case ERGBFormat::Invalid: return ERGBFormat::Invalid;
	case ERGBFormat::RGBA: return ERGBFormat::BGRA; // return BGRA for RGBA input
	case ERGBFormat::BGRA: return ERGBFormat::BGRA;
	case ERGBFormat::Gray: return ERGBFormat::Gray;
	default: return ERGBFormat::Invalid;
	}
}

EPixelFormat PixelFormatFromRGB(ERGBFormat Format, int32 BitDepth)
{
	check(BitDepth == 8 || BitDepth == 16) check(Format != ERGBFormat::RGBA)

		switch (Format)
	{
	case ERGBFormat::BGRA: return PF_B8G8R8A8;
	case ERGBFormat::Gray: return BitDepth == 8 ? PF_G8 : PF_G16;
	default: return PF_Unknown;
	}
}

FTextureSettings GetTextureSettings(const FString& Key, ERGBFormat Format)
{
	if (Key == L"normalMap")
	{
		return {false, TC_Normalmap};
	}
	if (Key == L"roughnessMap" || Key == L"metallicMap")
	{
		return {false, TC_Masks};
	}
	return {Format == ERGBFormat::Gray ? false : true, TC_Default};
}

UTexture2D* CreateTexture(UObject* Outer, const TArray64<uint8>& Data, int32 SizeX, int32 SizeY, ERGBFormat Format, int32 BitDepth,
						  const FString& TextureKey, const FName& BaseName)
{
	const EPixelFormat PixelFormat = PixelFormatFromRGB(Format, BitDepth);
	const FTextureSettings Settings = GetTextureSettings(TextureKey, Format);

	const FName TextureName = MakeUniqueObjectName(Outer, UTexture2D::StaticClass(), BaseName);
	UTexture2D* NewTexture = NewObject<UTexture2D>(Outer, TextureName, RF_Transient);

	NewTexture->PlatformData = new FTexturePlatformData();
	NewTexture->PlatformData->SizeX = SizeX;
	NewTexture->PlatformData->SizeY = SizeY;
	NewTexture->PlatformData->PixelFormat = PixelFormat;
	NewTexture->CompressionSettings = Settings.Compression;
	NewTexture->SRGB = Settings.SRGB;

	// TODO implement DXT compression for proper handling of grayscale textures

	// Allocate first mipmap and upload the pixel data
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->PlatformData->Mips.Add(Mip);
	Mip->SizeX = SizeX;
	Mip->SizeY = SizeY;
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	void* TextureData = Mip->BulkData.Realloc(CalculateImageBytes(SizeX, SizeY, 0, PixelFormat));
	FMemory::Memcpy(TextureData, Data.GetData(), Data.Num());
	Mip->BulkData.Unlock();

	NewTexture->UpdateResource();
	return NewTexture;
}

UTexture2D* LoadTextureFromDisk(UObject* Outer, const FString& ImagePath, const FString& TextureKey)
{
	static IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

	if (!FPaths::FileExists(ImagePath))
	{
		UE_LOG(LogMaterialConversion, Error, TEXT("File not found: %s"), *ImagePath);
		return nullptr;
	}

	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *ImagePath))
	{
		UE_LOG(LogMaterialConversion, Error, TEXT("Failed to load file: %s"), *ImagePath);
		return nullptr;
	}
	const EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(FileData.GetData(), FileData.Num());
	if (ImageFormat == EImageFormat::Invalid)
	{
		UE_LOG(LogMaterialConversion, Error, TEXT("Unrecognized image file format: %s"), *ImagePath);
		return nullptr;
	}

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);

	if (!ImageWrapper.IsValid())
	{
		UE_LOG(LogMaterialConversion, Error, TEXT("Failed to create image wrapper for file: %s"), *ImagePath);
		return nullptr;
	}

	// Decompress the image data
	TArray64<uint8> RawData;
	ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num());
	const ERGBFormat Format = GetRequestedFormat(ImageWrapper->GetFormat());
	ImageWrapper->GetRaw(Format, ImageWrapper->GetBitDepth(), RawData);

	// Create the texture and upload the uncompressed image data
	const FString TextureBaseName = TEXT("T_") + FPaths::GetBaseFilename(ImagePath);
	return CreateTexture(Outer, RawData, ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), Format, ImageWrapper->GetBitDepth(), TextureKey,
						 FName(*TextureBaseName));
}

EBlendMode ChooseBlendModeFromOpacityMap(const UTexture2D& OpacityMap, bool& UseAlphaChannelOpacity)
{
	UseAlphaChannelOpacity = false;

	const EPixelFormat PixelFormat = OpacityMap.GetPixelFormat();
	check(PixelFormat == PF_B8G8R8A8 || PixelFormat == PF_G8 || PixelFormat == PF_G16);

	uint32 BlackPixels = 0;
	uint32 WhitePixels = 0;

	switch (PixelFormat)
	{
	case PF_B8G8R8A8:
	{
		const FColor* ImageData = static_cast<const FColor*>(OpacityMap.PlatformData->Mips[0].BulkData.LockReadOnly());

		// First check if the alpha channel is not empty to see if we should use it to determine the blend mode or the R channel (for RBG opacity)
		UseAlphaChannelOpacity = HasAlpha(ImageData, OpacityMap.GetSizeX(), OpacityMap.GetSizeY());

		// Now count the black and white pixels of the appropriate opacity map channel to determine the opacity mode
		CountOpacityMapPixels(ImageData, UseAlphaChannelOpacity, OpacityMap.GetSizeX(), OpacityMap.GetSizeY(), BlackPixels, WhitePixels);
		break;
	}
	case PF_G8:
	{
		const uint8* ImageData = static_cast<const uint8*>(OpacityMap.PlatformData->Mips[0].BulkData.LockReadOnly());
		CountOpacityMapPixels(ImageData, OpacityMap.GetSizeX(), OpacityMap.GetSizeY(), BlackPixels, WhitePixels);
		break;
	}
	case PF_G16:
	{
		const uint16* ImageData = static_cast<const uint16*>(OpacityMap.PlatformData->Mips[0].BulkData.LockReadOnly());
		CountOpacityMapPixels(ImageData, OpacityMap.GetSizeX(), OpacityMap.GetSizeY(), BlackPixels, WhitePixels);
		break;
	}
	default: check(0)
	}

	OpacityMap.PlatformData->Mips[0].BulkData.Unlock();

	const uint32 TotalPixels = OpacityMap.GetSizeX() * OpacityMap.GetSizeY();
	if (WhitePixels >= TotalPixels * OPACITY_THRESHOLD)
	{
		return BLEND_Opaque;
	}
	if (WhitePixels + BlackPixels >= TotalPixels * OPACITY_THRESHOLD)
	{
		return BLEND_Masked;
	}
	return BLEND_Translucent;
}

EBlendMode ChooseBlendMode(UTexture2D* OpacityMap, double Opacity, EBlendMode BlendMode, bool& UseAlphaChannelOpacity)
{
	if (Opacity < OPACITY_THRESHOLD)
	{
		return BLEND_Translucent;
	}
	else if (BlendMode == BLEND_Masked)
	{
		return BLEND_Masked;
	}
	else if (BlendMode == BLEND_Translucent && OpacityMap)
	{
		// OpacityMap exists and opacitymap.mode is blend (which is the default value) so we need to check the content of the OpacityMap
		// to really decide which material we need for Unreal
		return ChooseBlendModeFromOpacityMap(*OpacityMap, UseAlphaChannelOpacity);
	}
	else
	{
		return BLEND_Opaque;
	}
}

EBlendMode GetBlendMode(const FString& OpacityMapMode)
{
	if (OpacityMapMode == "mask")
	{
		return BLEND_Masked;
	}
	else if (OpacityMapMode == "blend")
	{
		return BLEND_Translucent;
	}
	return BLEND_Opaque;
}

UMaterialInterface* GetMaterialByBlendMode(EBlendMode Mode, UMaterialInterface* Opaque, UMaterialInterface* Masked, UMaterialInterface* Translucent)
{
	switch (Mode)
	{
	case BLEND_Translucent: return Translucent;
	case BLEND_Masked: return Masked;
	default: return Opaque;
	}
}

} // namespace

namespace Vitruvio
{
UMaterialInstanceDynamic* GameThread_CreateMaterialInstance(UObject* Outer, UMaterialInterface* OpaqueParent, UMaterialInterface* MaskedParent,
															UMaterialInterface* TranslucentParent, const FMaterialContainer& MaterialContainer)
{
	check(IsInGameThread());

	TMap<FString, TFuture<UTexture2D*>> TextureProperties;

	// Load Textures Asynchronously
	for (const auto& TextureProperty : MaterialContainer.TextureProperties)
	{
		// Load textures async in thread pool
		// clang-format off
		TFuture<UTexture2D*> Result = Async(EAsyncExecution::ThreadPool, [Outer, TextureProperty]() -> UTexture2D* {
            QUICK_SCOPE_CYCLE_COUNTER(STAT_MaterialConversion_LoadTexture);
			if (TextureProperty.Value.IsEmpty())
			{
                return nullptr;
            }
            UTexture2D* Texture = LoadTextureFromDisk(Outer, TextureProperty.Value, TextureProperty.Key);
            return Texture;
        });
		// clang-format on

		TextureProperties.Add(TextureProperty.Key, MoveTemp(Result));
	}

	const float Opacity = MaterialContainer.ScalarProperties["opacity"];
	UTexture2D* OpacityMap = TextureProperties.Contains("opacityMap") ? TextureProperties["opacityMap"].Get() : nullptr;

	bool UseAlphaChannelOpacity = false;
	const EBlendMode ChosenBlendMode = ChooseBlendMode(OpacityMap, Opacity, GetBlendMode(MaterialContainer.BlendMode), UseAlphaChannelOpacity);

	const auto Parent = GetMaterialByBlendMode(ChosenBlendMode, OpaqueParent, MaskedParent, TranslucentParent);
	UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(Parent, Outer);

	// TODO implement in Material
	MaterialInstance->SetScalarParameterValue(FName(TEXT("opacitySource")), UseAlphaChannelOpacity);

	for (const TPair<FString, TFuture<UTexture2D*>>& TextureFuture : TextureProperties)
	{
		const auto Result = TextureFuture.Value.Get();
		MaterialInstance->SetTextureParameterValue(FName(TextureFuture.Key), Result);
	}
	for (const TPair<FString, double>& ScalarProperty : MaterialContainer.ScalarProperties)
	{
		MaterialInstance->SetScalarParameterValue(FName(ScalarProperty.Key), ScalarProperty.Value);
	}
	for (const TPair<FString, FLinearColor>& ColorProperty : MaterialContainer.ColorProperties)
	{
		MaterialInstance->SetVectorParameterValue(FName(ColorProperty.Key), ColorProperty.Value);
	}

	return MaterialInstance;
}
} // namespace Vitruvio
