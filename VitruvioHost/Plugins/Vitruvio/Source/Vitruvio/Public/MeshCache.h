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
#include "VitruvioMesh.h"

class FMeshCache
{
public:
	VITRUVIO_API TSharedPtr<FVitruvioMesh> Get(const FString& Uri);
	VITRUVIO_API TSharedPtr<FVitruvioMesh> InsertOrGet(const FString& Uri, const TSharedPtr<FVitruvioMesh>& Mesh);
	VITRUVIO_API void Empty();

private:
	FCriticalSection MeshCacheCriticalSection;

	TMap<FString, TSharedPtr<FVitruvioMesh>> Cache;
};
