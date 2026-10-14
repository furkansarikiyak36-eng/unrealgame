#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SimulationBudgetAllocator.generated.h"
UCLASS()
class MYPROJECT2_API USimulationBudgetAllocator : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxChunksPerFrame=8;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxTickMilliseconds=4.0f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxCellsPerChunk=256;
 UFUNCTION(BlueprintCallable,Category="Budget") void ResetFrame();
 UFUNCTION(BlueprintCallable,Category="Budget") bool TryAllocateChunk();
 UFUNCTION(BlueprintPure,Category="Budget") bool HasTimeRemaining() const;
 UFUNCTION(BlueprintPure,Category="Budget") int32 GetChunksProcessedThisFrame() const{return ChunksProcessed;}
protected: double FrameStartTime=0.0; int32 ChunksProcessed=0;
};
