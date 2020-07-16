// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

#include "PolygonWindings.h"

namespace
{
struct FWindingEdge
{
	const int32 Index0;
	const int32 Index1;

	int32 Count;

	FWindingEdge() : Index0(-1), Index1(-1), Count(0) {}

	FWindingEdge(const int32 Index0, const int32 Index1) : Index0(Index0), Index1(Index1), Count(1) {}

	bool operator==(const FWindingEdge& E) const { return (E.Index0 == Index0 && E.Index1 == Index1) || (E.Index0 == Index1 && E.Index1 == Index0); }
};

uint32 GetTypeHash(const FWindingEdge& Edge)
{
	// Use a modified version of cantor paring to make the hash function commutative. See
	// https://math.stackexchange.com/questions/882877/produce-unique-number-given-two-integers
	const int32 MaxIndex = FMath::Max(Edge.Index0, Edge.Index1);
	const int32 MinIndex = FMath::Min(Edge.Index0, Edge.Index1);
	return (MaxIndex * (MaxIndex + 1)) / 2 + MinIndex;
}

} // namespace

namespace Vitruvio
{
TArray<TArray<FVector>> GetOutsideWindings(const TArray<FVector>& InVertices, const TArray<int32>& InIndices)
{
	const int32 NumTriangles = InIndices.Num() / 3;

	// Create a list of edges and count the number of times they are used
	TSet<FWindingEdge> Edges;
	for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles; ++TriangleIndex)
	{
		for (int32 VertexIndex = 0; VertexIndex < 3; ++VertexIndex)
		{
			const int32 Index0 = InIndices[TriangleIndex * 3 + VertexIndex];
			const int32 Index1 = InIndices[TriangleIndex * 3 + (VertexIndex + 1) % 3];

			const FWindingEdge Edge(Index0, Index1);
			FWindingEdge* ExistingEdge = Edges.Find(Edge);
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

	// Only save edges which are used exactly once, this will leave edges at the outside of the shape
	TSortedMap<int32, FWindingEdge> EdgeMap;
	for (const FWindingEdge& Edge : Edges)
	{
		if (Edge.Count == 1)
		{
			EdgeMap.Add(Edge.Index0, Edge);
		}
	}

	// Organize the remaining edges in the list so that the vertices will meet up to form a continuous outline around the shapes
	TArray<TArray<FVector>> Windings;
	while (EdgeMap.Num() > 0)
	{
		TArray<FVector> WindingVertices;

		// Get and remove first edge
		auto EdgeIter = EdgeMap.CreateIterator();
		const FWindingEdge FirstEdge = EdgeIter.Value();
		EdgeIter.RemoveCurrent();

		WindingVertices.Add(InVertices[FirstEdge.Index0]);
		int NextIndex = FirstEdge.Index1;

		// Find connected edges
		while (EdgeMap.Contains(NextIndex))
		{
			const FWindingEdge& Current = EdgeMap.FindAndRemoveChecked(NextIndex);
			WindingVertices.Add(InVertices[Current.Index0]);
			NextIndex = Current.Index1;
		}

		Windings.Add(WindingVertices);
	}

	return Windings;
}
} // namespace Vitruvio
