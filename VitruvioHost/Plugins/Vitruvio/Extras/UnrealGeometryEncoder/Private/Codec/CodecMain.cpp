/* Copyright 2020 Esri
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

#include "CodecMain.h"

#include "Encoder/UnrealGeometryEncoder.h"

#include "prtx/ExtensionManager.h"

// TODO get version when we automatically download PRT from github and set in build script
namespace
{
const int VERSION_MAJOR = 2;
const int VERSION_MINOR = 7;
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
