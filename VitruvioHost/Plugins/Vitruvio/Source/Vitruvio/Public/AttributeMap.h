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

#pragma once

#include "RuleAttributes.h"

#include "CoreUObject.h"
#include "PRTTypes.h"

class VITRUVIO_API FAttributeMap
{
public:
	FAttributeMap() {}

	FAttributeMap(AttributeMapUPtr AttributeMap, RuleFileInfoUPtr RuleInfo) : AttributeMap(std::move(AttributeMap)), RuleInfo(std::move(RuleInfo)) {}

	TMap<FString, URuleAttribute*> ConvertToUnrealAttributeMap(UObject* const Outer);

private:
	const AttributeMapUPtr AttributeMap;
	const RuleFileInfoUPtr RuleInfo;
};

using FAttributeMapPtr = TSharedPtr<FAttributeMap>;
