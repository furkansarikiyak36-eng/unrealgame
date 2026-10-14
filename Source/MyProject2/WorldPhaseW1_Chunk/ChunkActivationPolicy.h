#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "ChunkActivationPolicy.generated.h"
UCLASS(BlueprintType)
class MYPROJECT2_API UChunkActivationPolicy : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Thresholds") float ContinuousRadiusMeters=100.0f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Thresholds") float ScheduledRadiusMeters=500.0f;
 UFUNCTION(BlueprintPure,Category="Policy") EChunkActivationMode DetermineMode(float DistanceMeters) const;
 UFUNCTION(BlueprintPure,Category="Policy") float GetUpdateIntervalSeconds(EChunkActivationMode Mode) const;
};
