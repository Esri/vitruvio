// Copyright 2019 - 2020 Esri. All Rights Reserved.

#pragma once

namespace Vitruvio
{
namespace PolygonWindings
{
	/**
	 * Takes a set of polygons and returns a vertex array representing the outside winding
	 * for them. This will work for convex or concave sets of polygons but not for concave polygons with holes.
	 *
	 * Note: This function is adapted from FPoly#GetOutsideWindings.
	 *
	 * @param InVertices		Input vertices
	 * @param InIndices		Input triangle indices
	 * @param InWindings		The resulting sets of vertices that represent the windings
	 */
	void GetOutsideWindings(const TArray<FVector>& InVertices, const TArray<int32>& InIndices, TArray<TArray<FVector>>& InWindings);
} // namespace PolygonWindings
} // namespace Vitruvio
