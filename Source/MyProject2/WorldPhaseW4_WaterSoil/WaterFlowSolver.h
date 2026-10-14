#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WaterLayer.h"
#include "WaterFlowSolver.generated.h"
UCLASS()
class MYPROJECT2_API UWaterFlowSolver : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float Gravity=9.81f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float FlowResistance=0.1f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float MinFlowThreshold=0.001f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxTransferFraction=0.25f;
 UFUNCTION(BlueprintCallable,Category="Flow") void SolveFlow(UWaterLayer* Layer,float DeltaTime);
protected: void FlowBetweenCells(FWaterCell& From,FWaterCell& To,float LevelDiff,float DeltaTime);
};
