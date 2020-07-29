// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "IImageWrapper.h"

namespace Vitruvio
{
int32 DetectChannels(EImageFormat ImageFormat, uint8* CompressedData, size_t CompressedSize);
}
