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

#include "InitialShape.h"

namespace Vitruvio
{

/**
 * Takes a triangulated input mesh (vertices and indices) and returns a polygon consisting of faces and holes.
 *
 * This will work for convex or concave polygons and polygons with holes.
 *
 * Note: This function is adapted from FPoly#GetOutsideWindings.
 *
 * @param InVertices	Input triangle vertices
 * @param InIndices		Input triangle indices
 */
FInitialShapePolygon GetPolygon(const TArray<FVector3f>& InVertices, const TArray<int32>& InIndices);

} // namespace Vitruvio
