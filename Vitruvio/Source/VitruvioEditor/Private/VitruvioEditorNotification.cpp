#include "CoreMinimal.h"
#include "ShaderCompiler.h"
#include "Widgets/Notifications/GlobalNotification.h"
#include "Widgets/Notifications/SNotificationList.h"

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
FVitruvioEditorNotification GShaderCompilingNotification;

bool FVitruvioEditorNotification::ShouldShowNotification(const bool bIsNotificationAlreadyActive) const
{
	return false;
}

void FVitruvioEditorNotification::SetNotificationText(const TSharedPtr<SNotificationItem>& InNotificationItem) const
{
	InNotificationItem->SetText(FText::FromString("Vitruvio Notification"));
}