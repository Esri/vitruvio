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

#include "Factories/Factory.h"

#include "RulePackageFactory.generated.h"

UCLASS(hidecategories = Object)
class URulePackageFactory final : public UFactory
{
	GENERATED_UCLASS_BODY()

	UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type,
								 const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;

	bool FactoryCanImport(const FString& Filename) override;

	bool ConfigureProperties() override;
};
