#include "InitialShape.h"

TArray<FVector> FInitialShapeData::GetVertices() const
{
	TArray<FVector> AllVertices;
	for (const TArray<FVector>& Vertices : FaceVertices)
	{
		AllVertices.Append(Vertices);
	}
	return AllVertices;
}
