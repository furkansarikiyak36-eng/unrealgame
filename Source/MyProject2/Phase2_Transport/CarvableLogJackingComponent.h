#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CarvableLogJackingComponent.generated.h"

class ACarvableLog;
class USceneComponent;

/**
 * Player-facing helper for lifting one end of an installed carvable log and supporting it with a real 75 cm module.
 * The support is committed only if StructureLink confirms that the raised log has a valid ground path.
 */
UCLASS(ClassGroup = (Construction), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UCarvableLogJackingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCarvableLogJackingComponent();

	UFUNCTION(BlueprintCallable, Category = "Construction|Jacking")
	bool LiftOneEnd(ACarvableLog* Log, float RaiseHeightCm, bool bRaiseEndB = true);

	UFUNCTION(BlueprintCallable, Category = "Construction|Jacking")
	bool PlaceSupportUnder(ACarvableLog* ElevatedLog, ACarvableLog* SupportPiece, bool bRaiseEndB = true);

	UFUNCTION(BlueprintPure, Category = "Construction|Jacking")
	bool IsFullySupported(const ACarvableLog* Log) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Jacking", meta = (ClampMin = "0.0"))
	float MaxLiftHeightCm = 75.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Jacking", meta = (ClampMin = "1.0"))
	float MaxPitchDegrees = 75.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Jacking", meta = (ClampMin = "0.0"))
	float GroundTraceUpCm = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Jacking", meta = (ClampMin = "1.0"))
	float GroundTraceDownCm = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Jacking", meta = (ClampMin = "0.0"))
	float SupportHeightToleranceCm = 10.f;

private:
	bool GetEndpointGeometry(
		const ACarvableLog* Log,
		bool bRaiseEndB,
		FVector& OutEndpointCenter,
		FVector& OutEndpointAxis,
		float& OutRadius) const;
};
