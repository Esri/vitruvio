#include "MeshCache.h"

TSharedPtr<FVitruvioMesh> FMeshCache::Get(const FString& Uri)
{
	FScopeLock Lock(&MeshCacheCriticalSection);
	const auto Result = Cache.Find(Uri);

	return Result ? *Result : TSharedPtr<FVitruvioMesh> {};
}

TSharedPtr<FVitruvioMesh> FMeshCache::InsertOrGet(const FString& Uri, const TSharedPtr<FVitruvioMesh>& Mesh)
{
	FScopeLock Lock(&MeshCacheCriticalSection);
	const auto Result = Cache.Find(Uri);
	if (Result)
	{
		return *Result;
	}
	Cache.Add(Uri, Mesh);
	return Mesh;
}

void FMeshCache::Empty()
{
	FScopeLock Lock(&MeshCacheCriticalSection);
	Cache.Empty();
}
