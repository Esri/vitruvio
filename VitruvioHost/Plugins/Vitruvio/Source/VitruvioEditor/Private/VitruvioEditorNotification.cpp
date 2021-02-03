/* Copyright 2021 Esri
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

	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FGlobalEditorNotification, STATGROUP_Tickables);
	}
	virtual void Tick(float DeltaTime) override
	{
		TickNotification(DeltaTime);
	}
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
			InNotificationItem->SetText(FText::FromString(FString::Printf(TEXT("Generating %d Models"), Vitruvio->GetNumGenerateCalls())));
		}
		else if (Vitruvio->IsLoadingRpks())
		{
			InNotificationItem->SetText(FText::FromString("Loading RPK"));
		}
	}
}