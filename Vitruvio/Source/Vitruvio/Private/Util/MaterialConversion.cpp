// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "MaterialConversion.h"

#include "PRTTypes.h"
#include "RuleAttributes.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImageCore/Public/ImageCore.h"

#include <map>

DEFINE_LOG_CATEGORY(LogMaterialConversion);

namespace
{

constexpr double BLACK_COLOR_THRESHOLD = 0.02;
constexpr double WHITE_COLOR_THRESHOLD = 1.0 - BLACK_COLOR_THRESHOLD;
constexpr double OPACITY_THRESHOLD = 0.98;

struct TextureSettings
{
	bool SRGB;
	TextureCompressionSettings Compression;
};

void CountOpacityMapPixels(const FColor* SrcColors, bool UseAlphaChannel, int32 SizeX, int32 SizeY, uint32& BlackPixels, uint32& WhitePixels)
{
	BlackPixels = 0;
	WhitePixels = 0;

	const FColor* LastColor = SrcColors + (SizeX * SizeY);
	while (SrcColors < LastColor)
	{
		const uint8 Value = UseAlphaChannel ? SrcColors->A : SrcColors->R;

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

bool HasAlpha(const FColor* SrcColors, int32 SizeX, int32 SizeY)
{
	const FColor* LastColor = SrcColors + (SizeX * SizeY);
	while (SrcColors < LastColor)
	{
		if (SrcColors->A < (1.0f - SMALL_NUMBER))
		{
			return true;
		}
		++SrcColors;
	}
	return false;
}

EPixelFormat PixelFormatFromRGB(ERGBFormat Format, int32 BitDepth)
{
	if (BitDepth != 8 && BitDepth != 16)
	{
		return PF_Unknown;
	}

	switch (Format)
	{
	case ERGBFormat::RGBA: return PF_R8G8B8A8;
	case ERGBFormat::BGRA: return PF_B8G8R8A8;
	case ERGBFormat::Gray: return BitDepth == 8 ? PF_G8 : PF_G16;
	default: return PF_Unknown;
	}
}

UTexture2D* CreateTexture(UObject* Outer, const TArray64<uint8>& Data, int32 SizeX, int32 SizeY, ERGBFormat Format, int32 BitDepth,
						  const TextureSettings& Settings, const FName& BaseName)
{
	const EPixelFormat PixelFormat = PixelFormatFromRGB(Format, BitDepth);

	const FName TextureName = MakeUniqueObjectName(Outer, UTexture2D::StaticClass(), BaseName);
	UTexture2D* NewTexture = NewObject<UTexture2D>(Outer, TextureName, RF_Transient);

	NewTexture->PlatformData = new FTexturePlatformData();
	NewTexture->PlatformData->SizeX = SizeX;
	NewTexture->PlatformData->SizeY = SizeY;
	NewTexture->PlatformData->PixelFormat = PixelFormat;
	NewTexture->CompressionSettings = Settings.Compression;
	NewTexture->SRGB = Settings.SRGB;

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

UTexture2D* LoadTextureFromDisk(UObject* Outer, const FString& ImagePath, const TextureSettings& Settings)
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
	ImageWrapper->GetRaw(ImageWrapper->GetFormat(), ImageWrapper->GetBitDepth(), RawData);

	// Create the texture and upload the uncompressed image data
	const FString TextureBaseName = TEXT("T_") + FPaths::GetBaseFilename(ImagePath);
	return CreateTexture(Outer, RawData, ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), ImageWrapper->GetFormat(), ImageWrapper->GetBitDepth(),
						 Settings, FName(*TextureBaseName));
}

UTexture2D* GetTexture(UObject* Outer, const prt::AttributeMap* MaterialAttributes, const TextureSettings& Settings, wchar_t const* Key)
{
	size_t ValuesCount = 0;
	wchar_t const* const* Values = MaterialAttributes->getStringArray(Key, &ValuesCount);
	for (int ValueIndex = 0; ValueIndex < ValuesCount; ++ValueIndex)
	{
		std::wstring TextureUri(Values[ValueIndex]);
		if (TextureUri.size() > 0)
		{
			return LoadTextureFromDisk(Outer, FString(WCHAR_TO_TCHAR(Values[ValueIndex])), Settings);
		}
	}
	return nullptr;
}

EBlendMode ChooseBlendModeFromOpacityMap(const UTexture2D& OpacityMap)
{
	const FColor* ImageData = static_cast<const FColor*>(OpacityMap.PlatformData->Mips[0].BulkData.LockReadOnly());

	// First check if the alpha channel is not empty to see if we should use it to determine the blend mode or the R channel (for RBG opacity maps as
	// well as gray scale) Note that we need to do this because Unreal does not differentiate between RGB and RGBA formats and always uses alpha
	// channels
	const bool UseAlphaChannel = HasAlpha(ImageData, OpacityMap.GetSizeX(), OpacityMap.GetSizeY());

	// Now count the black and white pixels of the appropriate opacity map channel to determine the opacity mode
	uint32 BlackPixels = 0;
	uint32 WhitePixels = 0;
	CountOpacityMapPixels(ImageData, UseAlphaChannel, OpacityMap.GetSizeX(), OpacityMap.GetSizeY(), BlackPixels, WhitePixels);

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

EBlendMode ChooseBlendMode(UTexture2D* OpacityMap, double Opacity, EBlendMode BlendMode)
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
		return ChooseBlendModeFromOpacityMap(*OpacityMap);
	}
	else
	{
		return BLEND_Opaque;
	}
}

EBlendMode GetBlendMode(const prt::AttributeMap* MaterialAttributes)
{
	const wchar_t* OpacityMapMode = MaterialAttributes->getString(L"opacityMap.mode");
	if (OpacityMapMode)
	{
		const std::wstring ModeString(OpacityMapMode);
		if (ModeString == L"mask")
		{
			return EBlendMode::BLEND_Masked;
		}
		else if (ModeString == L"blend")
		{
			return EBlendMode::BLEND_Translucent;
		}
	}
	return EBlendMode::BLEND_Opaque;
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

FLinearColor GetLinearColor(const prt::AttributeMap* MaterialAttributes, wchar_t const* Key)
{
	size_t count;
	const double* values = MaterialAttributes->getFloatArray(Key, &count);
	if (count < 3)
	{
		return FLinearColor();
	}
	const FColor Color(values[0] * 255.0, values[1] * 255.0, values[2] * 255.0);
	return FLinearColor(Color);
}

double GetScalar(const prt::AttributeMap* MaterialAttributes, wchar_t const* Key)
{
	return MaterialAttributes->getFloat(Key);
}

TextureSettings GetTextureSettings(const std::wstring Key)
{
	if (Key == L"normalMap")
	{
		return {false, TC_Normalmap};
	}
	if (Key == L"roughnessMap" || Key == L"metallicMap")
	{
		return {false, TC_Masks};
	}
	return {true, TC_Default};
}

enum class MaterialPropertyType
{
	TEXTURE,
	LINEAR_COLOR,
	SCALAR
};

// see prtx/Material.h
// clang-format off
const std::map<std::wstring, MaterialPropertyType> KeyToTypeMap = {
	{L"diffuseMap", 	MaterialPropertyType::TEXTURE},
	{L"opacityMap", 	MaterialPropertyType::TEXTURE},
	{L"emissiveMap", 	MaterialPropertyType::TEXTURE},
	{L"metallicMap", 	MaterialPropertyType::TEXTURE},
	{L"roughnessMap", 	MaterialPropertyType::TEXTURE},
	{L"normalMap", 		MaterialPropertyType::TEXTURE},

	{L"diffuseColor", 	MaterialPropertyType::LINEAR_COLOR},
	{L"emissiveColor", 	MaterialPropertyType::LINEAR_COLOR},

	{L"metallic", 		MaterialPropertyType::SCALAR},
	{L"opacity", 		MaterialPropertyType::SCALAR},
	{L"roughness", 		MaterialPropertyType::SCALAR},
};
// clang-format on
} // namespace

namespace Vitruvio
{
UMaterialInstanceDynamic* GameThread_CreateMaterialInstance(UObject* Outer, UMaterialInterface* OpaqueParent, UMaterialInterface* MaskedParent,
															UMaterialInterface* TranslucentParent, const prt::AttributeMap* MaterialAttributes)
{
	check(IsInGameThread());

	TMap<FString, TFuture<UTexture2D*>> TextureProperties;
	TMap<FString, FLinearColor> ColorProperties;
	TMap<FString, double> ScalarProperties;

	// Convert AttributeMap to material
	size_t KeyCount = 0;
	wchar_t const* const* Keys = MaterialAttributes->getKeys(&KeyCount);
	for (size_t KeyIndex = 0; KeyIndex < KeyCount; KeyIndex++)
	{
		const wchar_t* Key = Keys[KeyIndex];
		const std::wstring KeyString(Key);
		const auto TypeIter = KeyToTypeMap.find(KeyString);
		if (TypeIter != KeyToTypeMap.end())
		{
			const MaterialPropertyType Type = TypeIter->second;
			switch (Type)
			{
			case MaterialPropertyType::TEXTURE:
			{
				// Load textures async in thread pool
				// clang-format off
				TFuture<UTexture2D*> Result = Async(EAsyncExecution::ThreadPool, [Outer, MaterialAttributes, Key]() {
					QUICK_SCOPE_CYCLE_COUNTER(STAT_MaterialConversion_LoadTexture);
					UTexture2D* Texture = GetTexture(Outer, MaterialAttributes, GetTextureSettings(Key), Key);
					return Texture;
				});
				// clang-format on

				TextureProperties.Add(Key, MoveTemp(Result));
				break;
			}
			case MaterialPropertyType::LINEAR_COLOR: ColorProperties.Add(Key, GetLinearColor(MaterialAttributes, Key)); break;
			case MaterialPropertyType::SCALAR: ScalarProperties.Add(Key, GetScalar(MaterialAttributes, Key)); break;
			default:;
			}
		}
	}

	const float Opacity = ScalarProperties["opacity"];
	UTexture2D* OpacityMap = TextureProperties.Contains("opacityMap") ? TextureProperties["opacityMap"].Get() : nullptr;

	const EBlendMode ChosenBlendMode = ChooseBlendMode(OpacityMap, Opacity, GetBlendMode(MaterialAttributes));

	const auto Parent = GetMaterialByBlendMode(ChosenBlendMode, OpaqueParent, MaskedParent, TranslucentParent);
	UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(Parent, Outer);

	for (const TPair<FString, TFuture<UTexture2D*>>& TextureFuture : TextureProperties)
	{
		const auto Result = TextureFuture.Value.Get();
		MaterialInstance->SetTextureParameterValue(FName(TextureFuture.Key), Result);
	}
	for (const TPair<FString, double>& ScalarProperty : ScalarProperties)
	{
		MaterialInstance->SetScalarParameterValue(FName(ScalarProperty.Key), ScalarProperty.Value);
	}
	for (const TPair<FString, FLinearColor>& ColorProperty : ColorProperties)
	{
		MaterialInstance->SetVectorParameterValue(FName(ColorProperty.Key), ColorProperty.Value);
	}

	return MaterialInstance;
}
} // namespace Vitruvio
