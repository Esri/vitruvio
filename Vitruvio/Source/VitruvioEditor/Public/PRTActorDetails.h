#pragma once
#include "IDetailCustomization.h"
#include "PRTActor.h"

class FPRTActorDetails : public IDetailCustomization
{
public:
	FPRTActorDetails();
	~FPRTActorDetails();
	
    static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;
	
	void OnAttributesChanged(UObject* Object, struct FPropertyChangedEvent& Event);
	
private:
	TArray<TWeakObjectPtr<>> ObjectsBeingCustomized;
	TWeakPtr<IDetailLayoutBuilder> CachedDetailBuilder;
};
