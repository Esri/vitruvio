// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

#include "InitialShape.h"

TArray<FVector> UInitialShape::GetVertices() const
{
	TArray<FVector> AllVertices;
	for (const FInitialShapeFace& Face : Faces)
	{
		AllVertices.Append(Face.Vertices);
	}
	return AllVertices;
}
