#include "MeshCache.h"

TSharedPtr<FVitruvioMesh> FMeshCache::Get(const FString& Uri)
{
	FScopeLock Lock(&MeshCacheCriticalSection);
	if (!Cache.Contains(Uri))
	{
		return {};
	}
	return Cache[Uri];
}

TSharedPtr<FVitruvioMesh> FMeshCache::InsertOrGet(const FString& Uri, const TSharedPtr<FVitruvioMesh>& Mesh)
{
	FScopeLock Lock(&MeshCacheCriticalSection);
	if (Cache.Contains(Uri))
	{
		return Cache[Uri];
	}
	Cache.Add(Uri, Mesh);
	return Mesh;
}

void FMeshCache::Invalidate()
{
	FScopeLock Lock(&MeshCacheCriticalSection);
	for (auto& Entry : Cache)
	{
		Entry.Value->Invalidate();
	}

	Cache.Empty();
}
