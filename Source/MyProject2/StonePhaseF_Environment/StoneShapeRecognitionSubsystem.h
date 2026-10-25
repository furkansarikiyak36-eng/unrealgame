#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StoneShapeRecognitionSubsystem.generated.h"

class AStoneActor;

/**
 * Recognizes simple player-built stone patterns after a stone is installed.
 * The scan is event-driven and bounded to a local neighborhood, not run every frame.
 */
UCLASS()
class MYPROJECT2_API UStoneShapeRecognitionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Construction|Recognition")
	bool TryRecognizeManualFireplace(AStoneActor* NewlyInstalledStone);

	static bool EvaluateHearthRing(
		const TArray<FVector>& GroundPoints,
		FVector& OutCenter,
		float& OutRadiusCm,
		float& OutRmsRadialErrorCm,
		float& OutMaxAngularGapDegrees,
		float MinRadiusCm = 30.f,
		float MaxRadiusCm = 100.f,
		float MaxHeightRangeCm = 22.f,
		float MaxRelativeRadialError = 0.30f,
		float MaxAngularGapDegrees = 140.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Recognition", meta = (ClampMin = "5", ClampMax = "12"))
	int32 MinimumRingStones = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Recognition", meta = (ClampMin = "5", ClampMax = "16"))
	int32 MaximumRingStones = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Recognition", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float MinimumRingRadiusCm = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Recognition", meta = (ClampMin = "30.0", ClampMax = "160.0"))
	float MaximumRingRadiusCm = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Recognition", meta = (ClampMin = "0.0", ClampMax = "60.0"))
	float MaximumGroundHeightRangeCm = 22.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Recognition", meta = (ClampMin = "0.1", ClampMax = "0.5"))
	float MaximumRelativeRadialError = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Recognition", meta = (ClampMin = "90.0", ClampMax = "180.0"))
	float MaximumAngularGapDegrees = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Recognition", meta = (ClampMin = "100.0", ClampMax = "300.0"))
	float NeighborhoodRadiusCm = 210.f;

private:
	bool IsHearthAlreadyPresent(const FVector& Center) const;
};
