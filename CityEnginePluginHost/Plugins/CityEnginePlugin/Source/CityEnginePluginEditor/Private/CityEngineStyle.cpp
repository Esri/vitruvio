/* Copyright 2024 Esri
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

#include "CityEngineStyle.h"

#include "Interfaces/IPluginManager.h"
#include "SlateOptMacros.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FCityEngineStyle::StyleSet = nullptr;
TSharedPtr<ISlateStyle> FCityEngineStyle::Get()
{
	return StyleSet;
}

FName FCityEngineStyle::GetStyleSetName()
{
	static FName VitruvioStyleName(TEXT("VitruvioStyle"));
	return VitruvioStyleName;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FCityEngineStyle::Initialize()
{
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("CityEnginePlugin"))->GetBaseDir());
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	FSlateStyleSet* Style = StyleSet.Get();

	const FVector2D Icon64x64(64, 64);
	const FVector2D Icon16x16(16, 16);
	const FString ImagePath = Style->RootToContentDir(TEXT("Resources/CityEnginePlugin"), TEXT(".svg"));

	StyleSet->Set("ClassIcon.CityEngineActor", new FSlateVectorImageBrush(ImagePath, Icon16x16));
	StyleSet->Set("ClassThumbnail.CityEngineActor", new FSlateVectorImageBrush(ImagePath, Icon64x64));
	StyleSet->Set("ClassIcon.CityEngineComponent", new FSlateVectorImageBrush(ImagePath, Icon16x16));
	StyleSet->Set("ClassThumbnail.CityEngineComponent", new FSlateVectorImageBrush(ImagePath, Icon64x64));

	StyleSet->Set("ClassIcon.CityEngineBatchActor", new FSlateVectorImageBrush(ImagePath, Icon16x16));
	StyleSet->Set("ClassThumbnail.CityEngineBatchActor", new FSlateVectorImageBrush(ImagePath, Icon64x64));
	StyleSet->Set("ClassIcon.CityEngineBatchGridVisualizerActor", new FSlateVectorImageBrush(ImagePath, Icon16x16));
	StyleSet->Set("ClassThumbnail.CityEngineBatchGridVisualizerActor", new FSlateVectorImageBrush(ImagePath, Icon64x64));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
};

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef IMAGE_BRUSH_SVG

void FCityEngineStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}
