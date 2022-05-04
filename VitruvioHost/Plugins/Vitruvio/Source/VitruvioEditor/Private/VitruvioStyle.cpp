#include "VitruvioStyle.h"

#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "SlateOptMacros.h"

#define IMAGE_BRUSH_SVG( RelativePath, ... ) FSlateVectorImageBrush( Style->RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)

TSharedPtr<FSlateStyleSet> FVitruvioStyle::StyleSet = nullptr;
TSharedPtr<ISlateStyle> FVitruvioStyle::Get()
{
	return StyleSet;
}

FName FVitruvioStyle::GetStyleSetName()
{
	static FName VitruvioStyleName(TEXT("VitruvioStyle"));
	return VitruvioStyleName;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FVitruvioStyle::Initialize()
{
	const FVector2D Icon64x64(64, 64);
	const FVector2D Icon16x16(16, 16);
	
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("Vitruvio"))->GetBaseDir());
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	FSlateStyleSet* Style = StyleSet.Get();
	StyleSet->Set("ClassIcon.VitruvioActor", new IMAGE_BRUSH_SVG("Resources/Vitruvio", Icon16x16));
	StyleSet->Set("ClassThumbnail.VitruvioActor", new IMAGE_BRUSH_SVG("Resources/Vitruvio", Icon64x64));
	StyleSet->Set("ClassIcon.VitruvioComponent", new IMAGE_BRUSH_SVG("Resources/Vitruvio", Icon16x16));
	StyleSet->Set("ClassThumbnail.VitruvioComponent", new IMAGE_BRUSH_SVG("Resources/Vitruvio", Icon64x64));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
};

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef IMAGE_BRUSH_SVG

void FVitruvioStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}
