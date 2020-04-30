#pragma once
#include "IDetailCustomization.h"

class PRTActorDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TArray<TWeakObjectPtr<UObject>> ObjectsCustomized;
};
