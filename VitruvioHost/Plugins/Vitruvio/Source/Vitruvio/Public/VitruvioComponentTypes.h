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

#include "Report.h"
#include "VitruvioMesh.h"
#include "VitruvioModule.h"

struct FInstance
{
	FString Name;
	TSharedPtr<FVitruvioMesh> InstanceMesh;
	TArray<UMaterialInstanceDynamic*> OverrideMaterials;
	TArray<FTransform> Transforms;

	friend FORCEINLINE uint32 GetTypeHash(const FInstance& Request)
	{
		return GetTypeHash(Request.InstanceMesh->GetUri());
	}

	friend bool operator==(const FInstance& Lhs, const FInstance& Rhs)
	{
		return Lhs.InstanceMesh && Rhs.InstanceMesh ? Lhs.InstanceMesh->GetUri() == Rhs.InstanceMesh->GetUri() : false;
	}

	friend bool operator!=(const FInstance& Lhs, const FInstance& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

struct FConvertedGenerateResult
{
	TSharedPtr<FVitruvioMesh> ShapeMesh;
	TArray<FInstance> Instances;
	TMap<FString, FReport> Reports;
};

FConvertedGenerateResult BuildResult(const FGenerateResultDescription& GenerateResult,
									 TMap<Vitruvio::FMaterialAttributeContainer, UMaterialInstanceDynamic*>& MaterialCache,
									 TMap<FString, Vitruvio::FTextureData>& TextureCache,
									 TMap<UMaterialInterface*, FString>& MaterialIdentifiers,
									 TMap<FString, int32>& UniqueMaterialIdentifiers,
									 UMaterial* OpaqueParent, UMaterial* MaskedParent, UMaterial* TranslucentParent);


FString UniqueComponentName(const FString& Name, TMap<FString, int32>& UsedNames);
