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
