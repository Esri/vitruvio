// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "PolygonWindings.h"

namespace
{
struct FWindingEdge
{
	int32 Index0;
	int32 Index1;

	int32 Count;

	FWindingEdge() : Index0(-1), Index1(-1), Count(0) {}

	FWindingEdge(int32 Index0, int32 Index1) : Index0(Index0), Index1(Index1), Count(1) {}

	bool operator==(const FWindingEdge& E) const { return (E.Index0 == Index0 && E.Index1 == Index1) || (E.Index0 == Index1 && E.Index1 == Index0); }
};
} // namespace

namespace Vitruvio
{
namespace PolygonWindings
{

	TArray<TArray<FVector>> GetOutsideWindings(const TArray<FVector>& InVertices, const TArray<int32>& InIndices)
	{
		const int32 NumTriangles = InIndices.Num() / 3;

		// Create a list of edges and count the number of times they are used
		TArray<FWindingEdge> Edges;
		for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles; ++TriangleIndex)
		{
			for (int32 VertexIndex = 0; VertexIndex < 3; ++VertexIndex)
			{
				const int32 Index0 = InIndices[TriangleIndex * 3 + VertexIndex];
				const int32 Index1 = InIndices[TriangleIndex * 3 + (VertexIndex + 1) % 3];

				const FWindingEdge Edge(Index0, Index1);
				FWindingEdge* ExistingEdge = Edges.FindByKey(Edge);
				if (ExistingEdge)
				{
					ExistingEdge->Count++;
				}
				else
				{
					Edges.Add(Edge);
				}
			}
		}

		// Remove any edges from the list that are used more than once which will leave edges at the outside of the shape
		Edges.RemoveAll([](const FWindingEdge& E) { return E.Count > 1; });

		TSortedMap<int32, FWindingEdge> EdgeMap;
		for (const FWindingEdge& Edge : Edges)
		{
			EdgeMap.Add(Edge.Index0, Edge);
		}

		// Organize the remaining edges in the list so that the vertices will meet up to form a continuous outline around the shapes
		TArray<TArray<FVector>> Windings;
		while (EdgeMap.Num() > 0)
		{
			TArray<FWindingEdge> ConnectedEdges;

			// Get and remove first edge
			auto EdgeIter = EdgeMap.CreateIterator();
			FWindingEdge Current = EdgeIter.Value();
			EdgeIter.RemoveCurrent();
			ConnectedEdges.Add(Current);

			// Find connected edges
			while (EdgeMap.Contains(Current.Index1))
			{
				const FWindingEdge& Next = EdgeMap.FindAndRemoveChecked(Current.Index1);
				ConnectedEdges.Add(Next);
				Current = Next;
			}

			// Create the winding array
			TArray<FVector> WindingVertices;
			for (const auto& Edge : ConnectedEdges)
			{
				WindingVertices.Add(InVertices[Edge.Index0]);
			}

			Windings.Add(WindingVertices);
		}

		return Windings;
	}

} // namespace PolygonWindings
} // namespace Vitruvio
