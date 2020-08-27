// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "CodecMain.h"

#include "Encoder/UnrealGeometryEncoder.h"

#include "prtx/ExtensionManager.h"

// TODO get version when we automatically download PRT from github and set in build script
namespace
{
const int VERSION_MAJOR = 2;
const int VERSION_MINOR = 2;
} // namespace

extern "C"
{
	CODEC_EXPORTS_API void registerExtensionFactories(prtx::ExtensionManager* manager)
	{
		manager->addFactory(UnrealGeometryEncoderFactory::createInstance());
	}

	CODEC_EXPORTS_API void unregisterExtensionFactories(prtx::ExtensionManager* /*manager*/) {}

	CODEC_EXPORTS_API int getVersionMajor()
	{
		return VERSION_MAJOR;
	}

	CODEC_EXPORTS_API int getVersionMinor()
	{
		return VERSION_MINOR;
	}
}
