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

#include "prt/AttributeMap.h"

#include "CityEngineTypes.h"

#include "Misc/Variant.h"

#include "Report.generated.h"

UENUM(BlueprintType, DisplayName = "CityEngine Report Type")
enum class EReportPrimitiveType : uint8
{
	String,
	Float,
	Bool,
	Int,
	None
};

USTRUCT(Blueprintable, DisplayName = "CityEngine Report")
struct CITYENGINEPLUGIN_API FReport
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CityEngine")
	EReportPrimitiveType Type = EReportPrimitiveType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CityEngine")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CityEngine")
	FString Value;
};
