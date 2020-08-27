// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "ImageChannelsDetection.h"

#include "BmpImageSupport.h"

THIRD_PARTY_INCLUDES_START
#include "ThirdParty/libPNG/libPNG-1.5.2/png.h"
#include "ThirdParty/libPNG/libPNG-1.5.2/pnginfo.h"
#include "ThirdParty/zlib/zlib-1.2.5/Inc/zlib.h"
THIRD_PARTY_INCLUDES_END

namespace PNG
{

/**
 * Guard that safely releases PNG reader resources.
 */
class PNGReadGuard
{
public:
	PNGReadGuard(png_structp* InReadStruct, png_infop* InInfo) : png_ptr(InReadStruct), info_ptr(InInfo), PNGRowPointers(nullptr) {}

	~PNGReadGuard()
	{
		if (PNGRowPointers != nullptr)
		{
			png_free(*png_ptr, PNGRowPointers);
		}
		png_destroy_read_struct(png_ptr, info_ptr, nullptr);
	}

	void SetRowPointers(png_bytep* InRowPointers)
	{
		PNGRowPointers = InRowPointers;
	}

private:
	png_structp* png_ptr;
	png_infop* info_ptr;
	png_bytep* PNGRowPointers;
};

class PNGHeaderReader
{

public:
	PNGHeaderReader(uint8* CompressedData, int64 CompressedSize) : CompressedData(CompressedData), CompressedSize(CompressedSize), ReadOffset(0) {}

	uint8* CompressedData;
	int64 CompressedSize;

	int64 ReadOffset;

	uint8 GetNumChannels();
};

void* user_malloc(png_structp /*png_ptr*/, png_size_t size)
{
	check(size > 0);
	return FMemory::Malloc(size);
}

void user_free(png_structp /*png_ptr*/, png_voidp struct_ptr)
{
	check(struct_ptr);
	FMemory::Free(struct_ptr);
}

void user_read_compressed(png_structp png_ptr, png_bytep data, png_size_t length)
{
	PNGHeaderReader* ctx = static_cast<PNGHeaderReader*>(png_get_io_ptr(png_ptr));
	if (ctx->ReadOffset + static_cast<int64>(length) <= ctx->CompressedSize)
	{
		FMemory::Memcpy(data, &ctx->CompressedData[ctx->ReadOffset], length);
		ctx->ReadOffset += length;
	}
}

uint8 PNGHeaderReader::GetNumChannels()
{
	check(CompressedSize);

	png_structp png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, this, nullptr, nullptr, nullptr, user_malloc, user_free);
	check(png_ptr);

	png_infop info_ptr = png_create_info_struct(png_ptr);
	check(info_ptr);

	PNGReadGuard PNGGuard(&png_ptr, &info_ptr);
	{
		png_set_read_fn(png_ptr, this, user_read_compressed);

		png_read_info(png_ptr, info_ptr);

		return info_ptr->channels;
	}
}

} // namespace PNG

namespace BMP
{
int32 GetNumChannels(uint8* CompressedData)
{
	const EBitmapHeaderVersion HeaderVersion = EBitmapHeaderVersion::BHV_BITMAPINFOHEADER;
	const FBmiColorsMask* ColorMask = reinterpret_cast<FBmiColorsMask*>(CompressedData + sizeof(FBitmapFileHeader) + sizeof(FBitmapInfoHeader));
	const bool bHasAlphaChannel = ColorMask->RGBAMask[3] != 0 && HeaderVersion >= EBitmapHeaderVersion::BHV_BITMAPV4HEADER;
	return bHasAlphaChannel ? 4 : 3;
}
} // namespace BMP

namespace Vitruvio
{
int32 DetectChannels(EImageFormat ImageFormat, uint8* CompressedData, size_t CompressedSize)
{
	if (ImageFormat == EImageFormat::Invalid || CompressedData == nullptr)
	{
		return 0;
	}

	switch (ImageFormat)
	{
	case EImageFormat::JPEG:
		return 3;
	case EImageFormat::GrayscaleJPEG:
		return 1;
	case EImageFormat::PNG:
		return PNG::PNGHeaderReader(CompressedData, CompressedSize).GetNumChannels();
	case EImageFormat::BMP:
		return BMP::GetNumChannels(CompressedData);
	default:
		return 0;
	}
}
} // namespace Vitruvio
