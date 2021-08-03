#pragma once
#include "VitruvioMesh.h"

class FMeshCache
{
public:
	TSharedPtr<FVitruvioMesh> Get(const FString& Uri);
	TSharedPtr<FVitruvioMesh> InsertOrGet(const FString& Uri, const TSharedPtr<FVitruvioMesh>& Mesh);

private:
	FCriticalSection MeshCacheCriticalSection;

	TMap<FString, TSharedPtr<FVitruvioMesh>> Cache;
};
