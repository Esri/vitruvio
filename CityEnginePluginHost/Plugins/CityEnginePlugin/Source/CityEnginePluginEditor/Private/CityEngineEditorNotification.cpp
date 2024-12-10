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

#include "CoreMinimal.h"
#include "CityEngineModule.h"
#include "Widgets/Notifications/GlobalNotification.h"
#include "Widgets/Notifications/SNotificationList.h"

class FCityEngineEditorNotification : public FGlobalNotification, public FTickableEditorObject
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
FCityEngineEditorNotification GCityEngineNotification;

bool FCityEngineEditorNotification::ShouldShowNotification(const bool bIsNotificationAlreadyActive) const
{
	FCityEngineModule* CityEngineModule = FCityEngineModule::GetUnchecked();
	return CityEngineModule && (CityEngineModule->IsGenerating() || CityEngineModule->IsLoadingRpks());
}

void FCityEngineEditorNotification::SetNotificationText(const TSharedPtr<SNotificationItem>& InNotificationItem) const
{
	FCityEngineModule* CityEngineModule = FCityEngineModule::GetUnchecked();
	if (CityEngineModule)
	{
		if (CityEngineModule->IsGenerating())
		{
			InNotificationItem->SetText(FText::FromString(FString::Printf(TEXT("Generating %d Models"), CityEngineModule->GetNumGenerateCalls())));
		}
		else if (CityEngineModule->IsLoadingRpks())
		{
			InNotificationItem->SetText(FText::FromString("Loading RPK"));
		}
	}
}