/* Copyright 2023 Esri
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

#include "Containers/Array.h"
#include "UObject/Object.h"
#include "UObject/ObjectSaveContext.h"

#include "RulePackage.generated.h"

UCLASS(BlueprintType, HideCategories = (Object))
class VITRUVIO_API URulePackage final : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient)
	TArray<uint8> Data;

	UPROPERTY()
	FString SourcePath;

	virtual void PreSave(FObjectPreSaveContext SaveContext) override
	{
		Super::PreSave(SaveContext);

		// Create a unique ID for this object which can be used by FLazyObjectPtr to reference loaded/unloaded objects
		// The ID would be automatically generated the first time a FLazyObjectPtr of this object is created,
		// but it will mark the object as dirty and require a save.
		FUniqueObjectGuid::GetOrCreateIDForObject(this);
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Super::Serialize(Ar);

		// We can not use TArray#BulkSerialize as it does not use the fast path if we are not cooking
		// This is an adapted version of bulk serialize without that limitation
		Data.CountBytes(Ar);
		if (Ar.IsLoading())
		{
			int32 NewArrayNum = 0;
			Ar << NewArrayNum;
			Data.Empty(NewArrayNum);
			Data.AddUninitialized(NewArrayNum);
			Ar.Serialize(Data.GetData(), NewArrayNum);
		}
		else if (Ar.IsSaving())
		{
			int32 ArrayNum = Data.Num();
			Ar << ArrayNum;
			Ar.Serialize(Data.GetData(), ArrayNum);
		}
	}
};
