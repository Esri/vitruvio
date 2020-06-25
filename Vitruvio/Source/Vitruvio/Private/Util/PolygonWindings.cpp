// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "PolygonWindings.h"
#include "Math/Edge.h"

namespace
{
bool FindConnected(const TArray<FEdge>& Edges, const FEdge& Edge, FEdge& OutConnected, SIZE_T& OutIndex)
{
	OutIndex = -1;

	for (int32 EdgeIndex = 0; EdgeIndex < Edges.Num(); ++EdgeIndex)
	{
		FEdge Next = Edges[EdgeIndex];

		const bool IsConnected = Edge.Vertex[1].Equals(Next.Vertex[0]);
		const bool InverseConnected = Edge.Vertex[1].Equals(Next.Vertex[1]);

		if (InverseConnected)
		{
			Exchange(Next.Vertex[0], Next.Vertex[1]);
		}

		if (IsConnected || InverseConnected)
		{
			OutConnected = Next;
			OutIndex = EdgeIndex;
			return true;
		}
	}

	return false;
}
} // namespace

namespace Vitruvio
{
namespace PolygonWindings
{
	void GetOutsideWindings(const TArray<FVector>& InVertices, const TArray<int32>& InIndices, TArray<TArray<FVector>>& InWindings)
	{
		InWindings.Empty();

		const int32 NumTriangles = InIndices.Num() / 3;

		// Generate a list of ordered edges that represent the outside winding
		TArray<FEdge> EdgePool;
		for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles; ++TriangleIndex)
		{
			// Create a list of edges that are in this shape and set their counts to the number of times they are used.
			for (int32 VertexIndex = 0; VertexIndex < 3; ++VertexIndex)
			{
				const int32 Index0 = InIndices[TriangleIndex * 3 + VertexIndex];
				const int32 Index1 = InIndices[TriangleIndex * 3 + (VertexIndex + 1) % 3];

				FEdge Edge(InVertices[Index0], InVertices[Index1]);

				int32 FoundIndex;
				if (EdgePool.Find(Edge, FoundIndex))
				{
					EdgePool[FoundIndex].Count++;
				}
				else
				{
					Edge.Count = 1;
					EdgePool.AddUnique(Edge);
				}
			}
		}

		// Remove any edges from the list that are used more than once which will leave edges at the outside of the shape
		EdgePool.RemoveAll([](const FEdge& E) { return E.Count > 1; });

		// Organize the remaining edges in the list so that the vertices will meet up to form a continuous outline around the shape
		while (EdgePool.Num() > 0)
		{
			TArray<FEdge> ConnectedEdges;

			FEdge Current = EdgePool[0];
			EdgePool.RemoveAt(0);
			ConnectedEdges.Add(Current);

			SIZE_T ConnectedIndex;
			FEdge Next;
			while (FindConnected(EdgePool, Current, Next, ConnectedIndex))
			{
				ConnectedEdges.Add(Next);
				Current = Next;
				EdgePool.RemoveAt(ConnectedIndex);
			}

			// Create the winding array
			TArray<FVector> WindingVertices;
			for (const auto& Edge : ConnectedEdges)
			{
				WindingVertices.Add(Edge.Vertex[0]);
			}

			InWindings.Add(WindingVertices);
		}
	}
} // namespace PolygonWindings
} // namespace Vitruvio
