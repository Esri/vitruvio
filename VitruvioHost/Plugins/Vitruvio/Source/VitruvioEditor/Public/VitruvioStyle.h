#pragma once

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"
#include "Styling/SlateStyle.h"

class FVitruvioStyle
{
public:
	static void Initialize();
	static void Shutdown();

	static TSharedPtr<ISlateStyle> Get();

	static FName GetStyleSetName();
private:
	
	static TSharedPtr<FSlateStyleSet> StyleSet;
};
