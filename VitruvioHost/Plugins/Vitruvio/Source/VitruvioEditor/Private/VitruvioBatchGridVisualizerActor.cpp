#include "VitruvioBatchGridVisualizerActor.h"

#include "VitruvioBatchSubsystem.h"
#include "Subsystems/UnrealEditorSubsystem.h"

namespace
{
	int NumDebugGridLines = 50;
}

AVitruvioBatchGridVisualizerActor::AVitruvioBatchGridVisualizerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bLockLocation = true;
	bActorLabelEditable = false;
}

bool AVitruvioBatchGridVisualizerActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AVitruvioBatchGridVisualizerActor::Tick(float DeltaSeconds)
{
	if (UVitruvioBatchSubsystem* VitruvioBatchSubsystem = GetWorld()->GetSubsystem<UVitruvioBatchSubsystem>())
	{
		if (const AVitruvioBatchActor* BatchActor = VitruvioBatchSubsystem->GetBatchActor())
		{
			if (BatchActor->bDebugVisualizeGrid)
			{
				UUnrealEditorSubsystem* UnrealEditorSubsystem = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
				const FIntVector2 GridDimension = BatchActor->GridDimension;

				FVector CameraLocation;
				FRotator CameraRotation;
				UnrealEditorSubsystem->GetLevelViewportCameraInfo(CameraLocation, CameraRotation);

				const int CameraOffsetX = static_cast<int>(FMath::Floor(CameraLocation.X / GridDimension.X) * GridDimension.X);
				const int CameraOffsetY = static_cast<int>(FMath::Floor(CameraLocation.Y / GridDimension.Y) * GridDimension.Y);
				for (int32 X = -NumDebugGridLines; X < NumDebugGridLines; X++)
				{
					for (int32 Y = -NumDebugGridLines; Y < NumDebugGridLines; Y++)
					{
						FVector LineStartX(CameraOffsetX + X * GridDimension.X, CameraOffsetY + -NumDebugGridLines * GridDimension.Y, 0);
						FVector LineEndX(CameraOffsetX + X * GridDimension.X, CameraOffsetY + NumDebugGridLines * GridDimension.Y, 0);
						DrawDebugLine(GetWorld(), LineStartX, LineEndX, FColor::Red, false, 0, 0, 30);

						FVector LineStartY(CameraOffsetX + -NumDebugGridLines * GridDimension.X, CameraOffsetY + Y * GridDimension.Y, 0);
						FVector LineEndY(CameraOffsetX + NumDebugGridLines * GridDimension.X, CameraOffsetY + Y * GridDimension.Y, 0);
						DrawDebugLine(GetWorld(), LineStartY, LineEndY, FColor::Red, false, 0, 0, 30);
					}
				}
			}
		}
	}
}

bool AVitruvioBatchGridVisualizerActor::CanDeleteSelectedActor(FText& OutReason) const
{
	return false;
}
