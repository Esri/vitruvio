// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#pragma once

namespace Vitruvio
{
/**
 * Takes a set of polygons and returns a vertex array representing the outside winding.
 * This will work for convex or concave sets of polygons but not for concave polygons with holes.
 *
 * Note: This function is adapted from FPoly#GetOutsideWindings.
 *
 * @param InVertices	Input vertices
 * @param InIndices		Input triangle indices
 */
TArray<TArray<FVector>> GetOutsideWindings(const TArray<FVector>& InVertices, const TArray<int32>& InIndices);
} // namespace Vitruvio
