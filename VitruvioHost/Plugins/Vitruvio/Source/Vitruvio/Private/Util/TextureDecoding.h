#pragma once

#include "VitruvioTypes.h"

#include "prt/AttributeMap.h"

#include <memory>

namespace Vitruvio
{

struct FTextureMetadata
{
	size_t Width = 0;
	size_t Height = 0;
	size_t BytesPerBand = 0;
	size_t Bands = 0;

	EPixelFormat PixelFormat = EPixelFormat::PF_Unknown;
};

VITRUVIO_API FTextureMetadata ParseTextureMetadata(const prt::AttributeMap* TextureMetadata);

VITRUVIO_API FTextureData DecodeTexture(UObject* Outer, const FString& Key, const FString& Path, const FTextureMetadata& TextureMetadata,
										std::unique_ptr<uint8_t[]> Buffer, size_t BufferSize);

} // namespace Vitruvio
