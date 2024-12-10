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

#pragma once

#include "CityEngineBatchActorDetails.h"

#include "CityEngineBatchActor.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailCustomization.h"

namespace
{
void AddGenerateButton(IDetailCategoryBuilder& RootCategory, ACityEngineBatchActor* CityEngineBatchActor)
{
	// clang-format off
	RootCategory.AddCustomRow(FText::FromString(L"Generate All"))
	.WholeRowContent()
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SButton)
			.Text(FText::FromString("Generate All"))
			.ContentPadding(FMargin(30, 2))
			.OnClicked_Lambda([CityEngineBatchActor]()
			{
				CityEngineBatchActor->GenerateAll();
				return FReply::Handled();
			})
		]
		.VAlign(VAlign_Fill)
	];
	// clang-format on
}
}

TSharedRef<IDetailCustomization> FCityEngineBatchActorDetails::MakeInstance()
{
	return MakeShareable(new FCityEngineBatchActorDetails);
}

void FCityEngineBatchActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	if (ObjectsBeingCustomized.Num() != 1)
	{
		return;
	}

	TWeakObjectPtr<> CustomizedObject = ObjectsBeingCustomized[0];
	ACityEngineBatchActor* VitruvioBatchActor = nullptr;
	if (CustomizedObject.IsValid())
	{
		VitruvioBatchActor = Cast<ACityEngineBatchActor>(CustomizedObject.Get());
	}

	if (!VitruvioBatchActor)
	{
		return;
	}
	IDetailCategoryBuilder& RootCategory = DetailBuilder.EditCategory("CityEngineBatchActor");
	AddGenerateButton(RootCategory, VitruvioBatchActor);
}
