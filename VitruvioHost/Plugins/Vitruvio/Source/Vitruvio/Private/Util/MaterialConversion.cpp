/* Copyright 2023 Esri
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

#include "MaterialConversion.h"
#include "Runtime/Engine/Public/TextureResource.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformFileManager.h"
#include "Runtime/ImageCore/Public/ImageCore.h"
#include "VitruvioModule.h"
#include "VitruvioTypes.h"

DEFINE_LOG_CATEGORY(LogMaterialConversion);

namespace
{

constexpr double BlackColorThreshold = 0.02;
constexpr double WhiteColorThreshold = 1.0 - BlackColorThreshold;
constexpr double OpacityThreshold = 0.98;

const FString CityEngineDefaultShaderName("CityEngineShader");
const FString CityEnginePBRShaderName("CityEnginePBRShader");

template <typename T, typename F>
void CountOpacityMapPixels(const T* SrcColors, int32 SizeX, int32 SizeY, uint32& BlackPixels, uint32& WhitePixels, F Accessor)
{
	BlackPixels = 0;
	WhitePixels = 0;

	const T* LastColor = SrcColors + (SizeX * SizeY);
	while (SrcColors < LastColor)
	{
		const float Value = Accessor(SrcColors);

		if (Value < BlackColorThreshold)
		{
			BlackPixels++;
		}
		else if (Value > WhiteColorThreshold)
		{
			WhitePixels++;
		}
		++SrcColors;
	}
}

void CountOpacityMapPixels(const FColor* SrcColors, bool UseAlphaChannel, int32 SizeX, int32 SizeY, uint32& BlackPixels, uint32& WhitePixels)
{
	return CountOpacityMapPixels(SrcColors, SizeX, SizeY, BlackPixels, WhitePixels,
								 [UseAlphaChannel](const FColor* Color) { return static_cast<float>(UseAlphaChannel ? Color->A : Color->R) / 0xFF; });
}

void CountOpacityMapPixels(const uint8* SrcColors, int32 SizeX, int32 SizeY, uint32& BlackPixels, uint32& WhitePixels)
{
	return CountOpacityMapPixels(SrcColors, SizeX, SizeY, BlackPixels, WhitePixels,
								 [](const uint8* Color) { return static_cast<float>(*Color) / 0xFF; });
}

void CountOpacityMapPixels(const uint16* SrcColors, int32 SizeX, int32 SizeY, uint32& BlackPixels, uint32& WhitePixels)
{
	return CountOpacityMapPixels(SrcColors, SizeX, SizeY, BlackPixels, WhitePixels,
								 [](const uint16* Color) { return static_cast<float>(*Color) / 0xFFFF; });
}

EBlendMode ChooseBlendModeFromOpacityMap(const Vitruvio::FTextureData& OpacityMapData, bool UseAlphaAsOpacity)
{
	const UTexture2D* OpacityMap = OpacityMapData.Texture;
	const EPixelFormat PixelFormat = OpacityMap->GetPixelFormat();
	check(PixelFormat == PF_B8G8R8A8 || PixelFormat == PF_A16B16G16R16 || PixelFormat == PF_FloatRGBA);

	uint32 BlackPixels = 0;
	uint32 WhitePixels = 0;

	// Now count the black and white pixels of the appropriate opacity map channel to determine the opacity mode
	const FTexturePlatformData* PlatformData = OpacityMap->GetPlatformData();
	switch (PixelFormat)
	{
	case PF_B8G8R8A8:
	{
		const FColor* ImageData = reinterpret_cast<const FColor*>(PlatformData->Mips[0].BulkData.LockReadOnly());
		CountOpacityMapPixels(ImageData, UseAlphaAsOpacity, OpacityMap->GetSizeX(), OpacityMap->GetSizeY(), BlackPixels, WhitePixels);
		break;
	}
	case PF_A16B16G16R16:
	{
		const uint16* ImageData = reinterpret_cast<const uint16*>(PlatformData->Mips[0].BulkData.LockReadOnly());
		CountOpacityMapPixels(ImageData, OpacityMap->GetSizeX(), OpacityMap->GetSizeY(), BlackPixels, WhitePixels);
		break;
	}
	default:
		check(0)
	}

	OpacityMap->GetPlatformData()->Mips[0].BulkData.Unlock();

	const uint32 TotalPixels = OpacityMap->GetSizeX() * OpacityMap->GetSizeY();
	if (WhitePixels >= TotalPixels * OpacityThreshold)
	{
		return BLEND_Opaque;
	}
	if (WhitePixels + BlackPixels >= TotalPixels * OpacityThreshold)
	{
		return BLEND_Masked;
	}
	return BLEND_Translucent;
}

EBlendMode ChooseBlendMode(const Vitruvio::FTextureData& OpacityMapData, double Opacity, EBlendMode BlendMode, bool UseAlphaAsOpacity)
{
	if (Opacity < OpacityThreshold)
	{
		return BLEND_Translucent;
	}
	else if (BlendMode == BLEND_Masked)
	{
		return BLEND_Masked;
	}
	else if (BlendMode == BLEND_Translucent && OpacityMapData.Texture && OpacityMapData.Texture->GetPixelFormat() != PF_FloatRGBA)
	// explicitly don't check FloatRGBA textures as they were converted from grayscale float16/32 textures which will never have alpha channels
	{
		// OpacityMap exists and opacitymap.mode is blend (which is the default value) so we need to check the content of the OpacityMap
		// to really decide which material we need for Unreal
		return ChooseBlendModeFromOpacityMap(OpacityMapData, UseAlphaAsOpacity);
	}
	else
	{
		return BLEND_Opaque;
	}
}

EBlendMode GetBlendMode(const FString& OpacityMapMode)
{
	if (OpacityMapMode == "mask")
	{
		return BLEND_Masked;
	}
	else if (OpacityMapMode == "blend")
	{
		return BLEND_Translucent;
	}
	return BLEND_Opaque;
}

UMaterialInterface* GetMaterialByBlendMode(EBlendMode Mode, UMaterialInterface* Opaque, UMaterialInterface* Masked, UMaterialInterface* Translucent)
{
	switch (Mode)
	{
	case BLEND_Translucent:
		return Translucent;
	case BLEND_Masked:
		return Masked;
	default:
		return Opaque;
	}
}

class FLoadTextureTask
{
	TPromise<Vitruvio::FTextureData> Promise;
	UObject* Outer;
	TMap<FString, Vitruvio::FTextureData>& Cache;
	FCriticalSection& CacheCriticalSection;

	FString ImagePath;
	FString TextureKey;

public:
	FLoadTextureTask(TPromise<Vitruvio::FTextureData>&& InPromise, UObject* Outer, TMap<FString, Vitruvio::FTextureData>& Cache,
					 FCriticalSection& CacheCriticalSection, const FString& ImagePath, const FString& TextureKey)
		: Promise(MoveTemp(InPromise)), Outer(Outer), Cache(Cache), CacheCriticalSection(CacheCriticalSection), ImagePath(ImagePath),
		  TextureKey(TextureKey)
	{
	}

	static const TCHAR* GetTaskName()
	{
		return TEXT("FLoadTextureTask");
	}
	FORCEINLINE static TStatId GetStatId()
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadResolveMapTask, STATGROUP_TaskGraphTasks);
	}

	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyBackgroundThreadNormalTask;
	}

	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::TrackSubsequents;
	}

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_MaterialConversion_LoadTexture);
		FTaskTagScope Scope(ETaskTag::EParallelRenderingThread);
		Vitruvio::FTextureData TextureData = VitruvioModule::Get().DecodeTexture(Outer, ImagePath, TextureKey);
		{
			FScopeLock CacheLock(&CacheCriticalSection);

			Cache.Add(ImagePath, TextureData);
		}

		Promise.SetValue(TextureData);
	}
};

} // namespace

namespace Vitruvio
{
UMaterialInstanceDynamic* GameThread_CreateMaterialInstance(UObject* Outer, const FString& Name, UMaterialInterface* OpaqueParent,
															UMaterialInterface* MaskedParent, UMaterialInterface* TranslucentParent,
															const FMaterialAttributeContainer& MaterialContainer,
															TMap<FString, FTextureData>& TextureCache)
{
	check(IsInGameThread());

	TMap<FString, FGraphEventRef> TexturePropertyTasks;
	TMap<FString, TFuture<FTextureData>> TextureProperties;

	FCriticalSection CacheCriticalSection;

	for (const auto& TextureProperty : MaterialContainer.TextureProperties)
	{
		const FString TexturePath = TextureProperty.Value;
		TPromise<FTextureData> Promise;
		TFuture<FTextureData> Future = Promise.GetFuture();

		bool ValidCache = false;
		{
			FScopeLock CacheLock(&CacheCriticalSection);

			auto Cached = TextureCache.Find(TexturePath);
			if (Cached)
			{
				auto FileSystemTimeStamp = FPlatformFileManager::Get().GetPlatformFile().GetAccessTimeStamp(*TexturePath);
				if (FileSystemTimeStamp > Cached->LoadTime)
				{
					// If the timestamp on the filesystem is newer than our cached version we have to evict it from the cache and reload the texture
					// because it has changed
					TextureCache.Remove(*TextureCache.FindKey(*Cached));
				}
				else
				{
					// Found a valid entry in the cache which we can just use
					Promise.SetValue(*Cached);
					ValidCache = true;
				}
			}
		}

		if (!ValidCache)
		{
			// No valid entry found in the cache so we have to load it from the disk
			auto LoadTextureTask = TexturePropertyTasks.Find(TexturePath);
			if (LoadTextureTask)
			{
				FGraphEventArray Prerequisites;
				Prerequisites.Add(*LoadTextureTask);
				TGraphTask<TAsyncGraphTask<FTextureData>>::CreateTask(&Prerequisites)
					.ConstructAndDispatchWhenReady(
						[&TextureCache, TexturePath, &CacheCriticalSection]() {
							FScopeLock CacheLock(&CacheCriticalSection);
							return TextureCache[TexturePath];
						},
						MoveTemp(Promise), ENamedThreads::AnyThread);
			}
			else if (!TexturePath.IsEmpty())
			{
				FGraphEventRef LoadTask = TGraphTask<FLoadTextureTask>::CreateTask().ConstructAndDispatchWhenReady(
					MoveTemp(Promise), Outer, TextureCache, CacheCriticalSection, TextureProperty.Value, TextureProperty.Key);
				TexturePropertyTasks.Add(TexturePath, LoadTask);
			}
			else
			{
				Promise.SetValue({});
			}
		}

		TextureProperties.Add(TextureProperty.Key, MoveTemp(Future));
	}
	const float Opacity = MaterialContainer.ScalarProperties["opacity"];
	const FTextureData OpacityMapData = TextureProperties.Contains("opacityMap") ? TextureProperties["opacityMap"].Get() : FTextureData{};
	const bool UseAlphaAsOpacity = OpacityMapData.Texture && OpacityMapData.NumChannels == 4;
	const EBlendMode ChosenBlendMode = ChooseBlendMode(OpacityMapData, Opacity, GetBlendMode(MaterialContainer.BlendMode), UseAlphaAsOpacity);

	const FString Shader = MaterialContainer.StringProperties["shader"];

	UMaterialInterface* Parent = nullptr;

	if (!Shader.IsEmpty() && Shader != CityEngineDefaultShaderName && Shader != CityEnginePBRShaderName)
	{
		const FString FileName = FPaths::GetBaseFilename(Shader);
		const FString ParentMaterialPath = Shader + TEXT(".") + FileName;
		Parent = LoadObject<UMaterialInterface>(Outer, *ParentMaterialPath);
	}
	if (!Parent)
	{
		Parent = GetMaterialByBlendMode(ChosenBlendMode, OpaqueParent, MaskedParent, TranslucentParent);
	}

	// Note that we do not want to use MakeUniqueObjectName here because it would add increasing numbers as a postfix to each material name which is
	// not necessary. In case two materials would have the same name (eg from different VitruvioActor which use the same texture/material name)
	// UMaterialInstanceDynamic::Create will make the names unique instead.
	UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(Parent, GetTransientPackage(), *Name);
	MaterialInstance->SetFlags(RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);

	MaterialInstance->SetScalarParameterValue(FName(TEXT("opacitySource")), UseAlphaAsOpacity);

	for (const TPair<FString, TFuture<FTextureData>>& TextureFuture : TextureProperties)
	{
		const auto Result = TextureFuture.Value.Get();
		MaterialInstance->SetTextureParameterValue(FName(TextureFuture.Key), Result.Texture);
	}
	for (const TPair<FString, double>& ScalarProperty : MaterialContainer.ScalarProperties)
	{
		MaterialInstance->SetScalarParameterValue(FName(ScalarProperty.Key), ScalarProperty.Value);
	}
	for (const TPair<FString, FLinearColor>& ColorProperty : MaterialContainer.ColorProperties)
	{
		MaterialInstance->SetVectorParameterValue(FName(ColorProperty.Key), ColorProperty.Value);
	}

	return MaterialInstance;
}
} // namespace Vitruvio
