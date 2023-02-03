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

#include "PolygonWindings.h"

#include "Engine/Polys.h"

namespace
{
struct FWindingEdge
{
	const int32 Index0;
	const int32 Index1;
	int32 Color;

	FWindingEdge() : Index0(-1), Index1(-1), Color(-1) {}

	FWindingEdge(const int32 Index0, const int32 Index1, const int32 Color) : Index0(Index0), Index1(Index1), Color(Color) {}

	bool operator==(const FWindingEdge& E) const
	{
		return (E.Index0 == Index0 && E.Index1 == Index1);
	}
};

struct FWinding
{
	TArray<int32> Indices;
	int Color;

	FWinding(const TArray<int32>& Indices, const int Color) : Indices(Indices), Color(Color) {}
};

uint32 GetTypeHash(const FWindingEdge& Edge)
{
	std::size_t Hash = 0x04C76972;
	Hash = HashCombine(Hash, Edge.Index0);
	return HashCombine(Hash, Edge.Index1);
}

void ColorEdges(FWindingEdge& Edge, TSortedMap<int32, TArray<FWindingEdge>>& EdgeMap, TSet<FWindingEdge>& Visited, int Color)
{
	if (Visited.Contains(Edge))
	{
		return;
	}

	Edge.Color = Color;
	Visited.Add(Edge);
	for (FWindingEdge& Connected : EdgeMap[Edge.Index0])
	{
		ColorEdges(Connected, EdgeMap, Visited, Color);
	}
	for (FWindingEdge& Connected : EdgeMap[Edge.Index1])
	{
		ColorEdges(Connected, EdgeMap, Visited, Color);
	}
}

bool PointInPolygon2D(const FVector3f& Point, const TArray<int32>& PolygonIndices, const TArray<FVector3f>& PolygonVertices)
{
	if (PolygonIndices.Num() < 3)
	{
		return false;
	}

	bool bIsInside = false;
	for (int Index = 0; Index < PolygonIndices.Num(); Index++)
	{
		const FVector3f& Current = PolygonVertices[PolygonIndices[Index]];
		const FVector3f& Next = PolygonVertices[PolygonIndices[Index + 1 >= PolygonIndices.Num() ? 0 : Index + 1]];
		if (Current.Y < Point.Y && Next.Y >= Point.Y || Next.Y < Point.Y && Current.Y >= Point.Y)
		{
			if (Current.X + (Point.Y - Current.Y) / (Next.Y - Current.Y) * (Next.X - Current.X) < Point.X)
			{
				bIsInside = !bIsInside;
			}
		}
	}

	return bIsInside;
}

bool IsInsideOf2D(const TArray<int32>& FaceA, const TArray<int32>& FaceB, const TArray<FVector3f>& Vertices)
{
	if (FaceB.Num() == 1)
	{
		return false;
	}

	for (int IndexA = 0; IndexA < FaceA.Num() - 1; IndexA++)
	{
		for (int IndexB = 0; IndexB < FaceB.Num() - 1; IndexB++)
		{
			FVector Intersection;
			const bool Intersected =
				FMath::SegmentIntersection2D(FVector(Vertices[FaceA[IndexA]]), FVector(Vertices[FaceA[IndexA + 1]]), FVector(Vertices[FaceB[IndexB]]),
											 FVector(Vertices[FaceB[IndexB + 1]]), Intersection);
			if (Intersected)
			{
				return false;
			}
		}
	}

	return PointInPolygon2D(Vertices[FaceA[0]], FaceB, Vertices);
}

} // namespace

namespace Vitruvio
{

FInitialShapePolygon GetPolygon(const TArray<FVector3f>& InVertices, const TArray<int32>& InIndices)
{
	// The algorithm works as follows:
	// 1. We construct a graph of all edges and "color" all connected edges. Note that we can have multiple faces (with holes) in a single polygon
	// and we need to be able to find which hole belongs to which face.
	// 2. We find and remove opposite edges, this will leave us with all edges at the outside of a face or a hole.
	// 3. We combine all connected edges which form either faces or holes. Note that the ordering is already correct. Holes will have opposite
	// ordering of their encircling face.
	// 4. We check which hole belongs to which face by projecting the face/hole onto the xy plane and then doing edge intersection tests.
	// This might break with certain non planar polygons but CityEngine handles holes in a similar way.

	const int32 NumTriangles = InIndices.Num() / 3;

	// Construct edges
	TSet<FWindingEdge> Edges;
	for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles; ++TriangleIndex)
	{
		for (int32 VertexIndex = 0; VertexIndex < 3; ++VertexIndex)
		{
			const int32 Index0 = InIndices[TriangleIndex * 3 + VertexIndex];
			const int32 Index1 = InIndices[TriangleIndex * 3 + (VertexIndex + 1) % 3];
			Edges.Add({Index0, Index1, -1});
		}
	}

	// Color connected edges
	TSortedMap<int32, TArray<FWindingEdge>> EdgeMap;
	for (const FWindingEdge& Edge : Edges)
	{
		EdgeMap.FindOrAdd(Edge.Index0).Add(Edge);
	}

	TSet<FWindingEdge> Visited;
	int CurrentColor = 0;
	for (auto& KeyValue : EdgeMap)
	{
		for (FWindingEdge& Edge : KeyValue.Value)
		{
			if (!Visited.Contains(Edge))
			{
				ColorEdges(Edge, EdgeMap, Visited, CurrentColor++);
			}
		}
	}

	// Remove opposite edges to only keep the outside of either a face or a hole
	TSortedMap<int32, FWindingEdge> WindingEdgeMap;
	for (auto& KeyValue : EdgeMap)
	{
		for (const FWindingEdge& Current : KeyValue.Value)
		{
			const FWindingEdge Opposite(Current.Index1, Current.Index0, Current.Color);
			if (!Edges.Contains(Opposite))
			{
				// Note that at this point there should not be multiple edges connected to a single vertex
				WindingEdgeMap.Add(Current.Index0, Current);
			}
		}
	}

	// Organize the remaining edges in the list so that the vertices will meet up to form a continuous outline of either a face or a hole
	TArray<FWinding> Windings;
	while (WindingEdgeMap.Num() > 0)
	{
		TArray<int32> WindingIndices;

		// Get and remove first edge
		auto EdgeIter = WindingEdgeMap.CreateIterator();
		const FWindingEdge FirstEdge = EdgeIter.Value();
		EdgeIter.RemoveCurrent();

		WindingIndices.Add(FirstEdge.Index0);
		int NextIndex = FirstEdge.Index1;

		// Find connected edges
		while (WindingEdgeMap.Contains(NextIndex))
		{
			const FWindingEdge& Current = WindingEdgeMap.FindAndRemoveChecked(NextIndex);
			WindingIndices.Add(Current.Index0);
			NextIndex = Current.Index1;
		}

		Windings.Add({WindingIndices, FirstEdge.Color});
	}

	// Find the relation between the faces
	TMap<int32, int32> InsideOf;
	for (int32 IndexA = 0; IndexA < Windings.Num(); IndexA++)
	{
		for (int32 IndexB = IndexA + 1; IndexB < Windings.Num(); IndexB++)
		{
			// No possible relation if they are not connected
			if (Windings[IndexA].Color != Windings[IndexB].Color)
			{
				continue;
			}

			if (IsInsideOf2D(Windings[IndexA].Indices, Windings[IndexB].Indices, InVertices))
			{
				InsideOf.Add(IndexA, IndexB);
			}
			else if (IsInsideOf2D(Windings[IndexB].Indices, Windings[IndexA].Indices, InVertices))
			{
				InsideOf.Add(IndexB, IndexA);
			}
		}
	}

	TSet<int32> HoleSet;
	TMap<int32, FInitialShapeFace> FaceMap;
	for (int32 FaceIndex = 0; FaceIndex < Windings.Num(); ++FaceIndex)
	{
		if (!InsideOf.Contains(FaceIndex))
		{
			FaceMap.Add(FaceIndex, FInitialShapeFace{Windings[FaceIndex].Indices});
		}
		else
		{
			HoleSet.Add(FaceIndex);
		}
	}

	for (int32 HoleIndex : HoleSet)
	{
		int32 InsideOfIndex = InsideOf[HoleIndex];
		if (FaceMap.Contains(InsideOfIndex))
		{
			FInitialShapeFace& ParentFace = FaceMap[InsideOfIndex];
			ParentFace.Holes.Add(FInitialShapeHole{Windings[HoleIndex].Indices});
		}
	}

	TArray<FInitialShapeFace> Faces;
	FaceMap.GenerateValueArray(Faces);
	FInitialShapePolygon Result = {Faces, InVertices};
	return Result;
}
} // namespace Vitruvio
