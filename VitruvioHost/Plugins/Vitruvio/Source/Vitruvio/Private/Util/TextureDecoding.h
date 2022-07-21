/* Copyright 2022 Esri
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
