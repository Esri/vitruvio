#include "CoreMinimal.h"
#include "ShaderCompiler.h"
#include "VitruvioModule.h"
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
	IModuleInterface* Module = FModuleManager::Get().LoadModule("Vitruvio");
	VitruvioModule* Vitruvio = dynamic_cast<VitruvioModule*>(Module);
	return Vitruvio && Vitruvio->GetState() == EPrtState::Downloading || Vitruvio->GetState() == EPrtState::Installing;
}

void FVitruvioEditorNotification::SetNotificationText(const TSharedPtr<SNotificationItem>& InNotificationItem) const
{
	IModuleInterface* Module = FModuleManager::Get().LoadModule("Vitruvio");
	VitruvioModule* Vitruvio = dynamic_cast<VitruvioModule*>(Module);
	if (Vitruvio)
	{
		EPrtState State = Vitruvio->GetState();
		if (State == EPrtState::Downloading)
		{
			if (Vitruvio->GetDownloadProgress())
			{
				const int32 DownloadProgress = static_cast<int32>(Vitruvio->GetDownloadProgress().GetValue() * 100);
				InNotificationItem->SetText(FText::FromString(FString::Printf(TEXT("Downloading PRT %d%%"), DownloadProgress)));
			}
			else
			{
				InNotificationItem->SetText(FText::FromString("Downloading PRT"));
			}
		}
		else if (State == EPrtState::Installing)
		{
			const int32 InstallProgress = static_cast<int32>(Vitruvio->GetInstallProgress() * 100);
			InNotificationItem->SetText(FText::FromString(FString::Printf(TEXT("Installing PRT %d%%"), InstallProgress)));
		}
	}
}