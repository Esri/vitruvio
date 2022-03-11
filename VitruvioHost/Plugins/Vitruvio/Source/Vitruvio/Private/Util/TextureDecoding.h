#pragma once

#include "VitruvioTypes.h"

#include "prt/AttributeMap.h"

#include <memory>

namespace Vitruvio
{

enum class EPRTPixelFormat
{
	GREY8,
	GREY16,
	FLOAT32,
	RGB8,
	RGBA8,
	Unknown
};

struct FTextureMetadata
{
	size_t Width = 0;
	size_t Height = 0;
	size_t BytesPerBand = 0;
	size_t Bands = 0;

	EPRTPixelFormat PixelFormat = EPRTPixelFormat::Unknown;
};

VITRUVIO_API FTextureMetadata ParseTextureMetadata(const prt::AttributeMap* TextureMetadata);

VITRUVIO_API FTextureData DecodeTexture(UObject* Outer, const FString& Key, const FString& Path, const FTextureMetadata& TextureMetadata,
										std::unique_ptr<uint8_t[]> Buffer, size_t BufferSize);

} // namespace Vitruvio
