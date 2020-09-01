// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

#include "IImageWrapper.h"

namespace Vitruvio
{
int32 DetectChannels(EImageFormat ImageFormat, uint8* CompressedData, size_t CompressedSize);
}
