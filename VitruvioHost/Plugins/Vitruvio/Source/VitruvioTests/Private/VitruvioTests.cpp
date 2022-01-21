// Copyright © 2017-2021 Esri R&D Center Zurich. All rights reserved.

#include "VitruvioTests.h"

#include "Modules/ModuleManager.h"
#include "Misc/AutomationTest.h"

#include "Vitruvio/Public/Util/PolygonWindings.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, VitruvioTests);

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolygonWindingsTest, "Esri.Vitruvio.PolygonWindings",
								 EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)


bool FPolygonWindingsTest::RunTest(const FString& Parameters)
{
	const TArray<FVector> Vertices = {{.0f, .0f, .0f}, {100.0f, .0f, .0f}, {100.0f, 100.0f, .0f}, {.0f, 100.0f, .0f}};
	const TArray<int32> Indices = {0, 1, 2, 2, 3, 0};

	const TArray<TArray<FVector>> Windings = Vitruvio::GetOutsideWindings(Vertices, Indices);
	TestEqual(TEXT("Must create single polygon"), Windings.Num(), 1);
	TestEqual(TEXT("Must have 4 vertices"), Windings[0].Num(), 4);
	for (int32 VertexIndex = 0; VertexIndex < Vertices.Num(); ++VertexIndex)
	{
		TestEqual(TEXT("Vertex must be equal"), Windings[0][VertexIndex], Vertices[VertexIndex], KINDA_SMALL_NUMBER);
	}
	return true;
}
