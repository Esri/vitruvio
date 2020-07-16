#include "CoreMinimal.h"
#include "VitruvioModule.h"
#include "Widgets/Notifications/GlobalNotification.h"
#include "Widgets/Notifications/SNotificationList.h"

namespace
{
VitruvioModule* GetVitruvioUnchecked()
{
	return FModuleManager::GetModulePtr<VitruvioModule>("Vitruvio");
}
} // namespace

class FVitruvioEditorNotification : public FGlobalNotification, public FTickableEditorObject
{
protected:
	virtual bool ShouldShowNotification(const bool bIsNotificationAlreadyActive) const override;
	virtual void SetNotificationText(const TSharedPtr<SNotificationItem>& InNotificationItem) const override;

	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }

	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FGlobalEditorNotification, STATGROUP_Tickables); }
	virtual void Tick(float DeltaTime) override { TickNotification(DeltaTime); }
};

/** Global notification object. */
FVitruvioEditorNotification GVitruvioNotification;

bool FVitruvioEditorNotification::ShouldShowNotification(const bool bIsNotificationAlreadyActive) const
{
	VitruvioModule* Vitruvio = GetVitruvioUnchecked();
	return Vitruvio && (Vitruvio->IsGenerating() || Vitruvio->IsLoadingRpks());
}

void FVitruvioEditorNotification::SetNotificationText(const TSharedPtr<SNotificationItem>& InNotificationItem) const
{
	VitruvioModule* Vitruvio = GetVitruvioUnchecked();
	if (Vitruvio)
	{
		if (Vitruvio->IsGenerating())
		{
			InNotificationItem->SetText(FText::FromString("Generating Models"));
		}
		else if (Vitruvio->IsLoadingRpks())
		{
			InNotificationItem->SetText(FText::FromString("Loading RPK"));
		}
	}
}