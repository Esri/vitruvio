#pragma once

#include "MaterialConversion.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "PRTTypes.h"
#include "RuleAttributes.h"
#include <map>

DEFINE_LOG_CATEGORY(LogMaterialConversion);

namespace
{
	struct TextureSettings
	{
		bool SRGB;
		TextureCompressionSettings Compression;
	};

	UTexture2D* CreateTexture(UObject* Outer, const TArray<uint8>& PixelData, int32 InSizeX, int32 InSizeY, const TextureSettings& Settings, EPixelFormat Format, FName BaseName)
	{
		// Shamelessly copied from UTexture2D::CreateTransient with a few modifications
		if (InSizeX <= 0 || InSizeY <= 0 ||
			(InSizeX % GPixelFormats[Format].BlockSizeX) != 0 ||
			(InSizeY % GPixelFormats[Format].BlockSizeY) != 0)
		{
			UE_LOG(LogMaterialConversion, Warning, TEXT("Invalid parameters"));
			return nullptr;
		}

		// Most important difference with UTexture2D::CreateTransient: we provide the new texture with a name and an owner
		const FName TextureName = MakeUniqueObjectName(Outer, UTexture2D::StaticClass(), BaseName);
		UTexture2D* NewTexture = NewObject<UTexture2D>(Outer, TextureName, RF_Transient);

		NewTexture->PlatformData = new FTexturePlatformData();
		NewTexture->PlatformData->SizeX = InSizeX;
		NewTexture->PlatformData->SizeY = InSizeY;
		NewTexture->PlatformData->PixelFormat = Format;
		NewTexture->CompressionSettings = Settings.Compression;
		NewTexture->SRGB = Settings.SRGB;

		// Allocate first mipmap and upload the pixel data
		FTexture2DMipMap* Mip = new FTexture2DMipMap();
		NewTexture->PlatformData->Mips.Add(Mip);
		Mip->SizeX = InSizeX;
		Mip->SizeY = InSizeY;
		Mip->BulkData.Lock(LOCK_READ_WRITE);
		void* TextureData = Mip->BulkData.Realloc(CalculateImageBytes(InSizeX, InSizeY, 0, Format));
		FMemory::Memcpy(TextureData, PixelData.GetData(), PixelData.Num());
		Mip->BulkData.Unlock();

		NewTexture->UpdateResource();
		return NewTexture;
	}

	EPixelFormat GetPixelFormatFromRGBFormat(ERGBFormat Format)
	{
		switch (Format)
		{
		case ERGBFormat::RGBA: return PF_R8G8B8A8;
		case ERGBFormat::BGRA: return PF_B8G8R8A8;
		case ERGBFormat::Gray: return PF_G8;
		default: return PF_Unknown;
		}
	}

	UTexture2D* LoadImageFromDisk(UObject* Outer, const FString& ImagePath, const TextureSettings& Settings)
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
		TArray<uint8> RawData;
		ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num());
		ImageWrapper->GetRaw(ImageWrapper->GetFormat(), ImageWrapper->GetBitDepth(), RawData);

		// Create the texture and upload the uncompressed image data
		const FString TextureBaseName = TEXT("T_") + FPaths::GetBaseFilename(ImagePath);
		return CreateTexture(Outer, RawData, ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), Settings, GetPixelFormatFromRGBFormat(ImageWrapper->GetFormat()), FName(*TextureBaseName));
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
				return LoadImageFromDisk(Outer, FString(Values[ValueIndex]), Settings);
			}
		}
		return nullptr;
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

	float GetScalar(const prt::AttributeMap* MaterialAttributes, wchar_t const* Key)
	{
		return MaterialAttributes->getFloat(Key);
	}

	TextureSettings GetTextureSettings(const std::wstring Key)
	{
		if (Key == L"normalMap")
		{
			return { false, TC_Normalmap };
		}
		else if (Key == L"roughnessMap" || Key == L"metallicMap")
		{
			return { false, TC_Masks };
		}
		return { true, TC_Default };
	}

	enum MaterialPropertyType
	{
		TEXTURE, LINEAR_COLOR, SCALAR
	};

	// see prtx/Material.h
	const std::map<std::wstring, MaterialPropertyType> KeyToTypeMap = {
		{L"diffuseMap", 	TEXTURE},
		{L"opacityMap", 	TEXTURE},
		{L"emissiveMap", 	TEXTURE},
		{L"metallicMap", 	TEXTURE},
		{L"roughnessMap", 	TEXTURE},
		{L"normalMap", 		TEXTURE},

		{L"diffuseColor", 	LINEAR_COLOR},
		{L"emissiveColor", 	LINEAR_COLOR},

		{L"metallic", 	SCALAR},
		{L"opacity", 	SCALAR},
		{L"roughness", 	SCALAR},
	};
}

UMaterialInstanceDynamic* CreateMaterialInstance(UObject* Outer, UMaterialInterface* OpaqueParent, UMaterialInterface* MaskedParent, UMaterialInterface* TranslucentParent, const prt::AttributeMap* MaterialAttributes)
{
	const auto BlendMode = GetBlendMode(MaterialAttributes);
	const auto Parent = GetMaterialByBlendMode(BlendMode, OpaqueParent, MaskedParent, TranslucentParent);
	UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(Parent, Outer);

	// Convert AttributeMap to material
	size_t KeyCount = 0;
	wchar_t const* const* Keys = MaterialAttributes->getKeys(&KeyCount);
	for (int KeyIndex = 0; KeyIndex < KeyCount; KeyIndex++)
	{
		const wchar_t* Key = Keys[KeyIndex];
		const std::wstring KeyString(Keys[KeyIndex]);
		const auto TypeIter = KeyToTypeMap.find(KeyString);
		if (TypeIter != KeyToTypeMap.end())
		{
			const MaterialPropertyType Type = TypeIter->second;
			switch (Type)
			{
				// TODO loading the texture should probably not happen in the game thread
			case TEXTURE:
				MaterialInstance->SetTextureParameterValue(FName(Key), GetTexture(Outer, MaterialAttributes, GetTextureSettings(Key), Key));
				break;
			case LINEAR_COLOR:
				MaterialInstance->SetVectorParameterValue(FName(Key), GetLinearColor(MaterialAttributes, Key));
				break;
			case SCALAR:
				MaterialInstance->SetScalarParameterValue(FName(Key), GetScalar(MaterialAttributes, Key));
				break;
			default:;
			}
		}
	}

	return MaterialInstance;
}