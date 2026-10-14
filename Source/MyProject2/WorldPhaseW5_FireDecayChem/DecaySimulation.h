#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DecaySimulation.generated.h"
class UChunkState;
UCLASS()
class MYPROJECT2_API UDecaySimulation : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseDecayRate=0.02f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float TemperatureFactor=0.5f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float MoistureFactor=0.7f;
 UFUNCTION(BlueprintCallable,Category="Decay") void SimulateChunk(UChunkState* Chunk,float DeltaDays,float Temperature);
};
