#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "WaterLayer.h"
#include "WaterFlowSolver.h"
#include "WaterSimulation.generated.h"
class UChunkState;
UCLASS()
class MYPROJECT2_API UWaterSimulation : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UWaterFlowSolver> FlowSolver;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float CellAreaM2=16.0f;
 UFUNCTION(BlueprintCallable,Category="Water") void SimulateChunk(UChunkState* Chunk,float DeltaDays);
 UFUNCTION(BlueprintCallable,Category="Water") void OnWaterLevelChanged(UChunkState* Chunk,int32 CellIndex,float OldLevel,float NewLevel);
 UFUNCTION(BlueprintCallable,Category="Water") void ApplyRainfall(UChunkState* Chunk,float RainfallMm);
protected: void BroadcastEventIfNeeded(UChunkState* Chunk,EWorldEventType Type,float Magnitude);
};
