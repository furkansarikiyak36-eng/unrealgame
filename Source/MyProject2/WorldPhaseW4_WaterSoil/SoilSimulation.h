#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SoilSimulation.generated.h"
class UChunkState;
UCLASS()
class MYPROJECT2_API USoilSimulation : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float InfiltrationRate=0.1f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float EvaporationRate=0.02f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float PercolationRate=0.01f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float NutrientNitrogenLossPerDay=0.05f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float NutrientPhosphorusLossPerDay=0.02f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float NutrientPotassiumLossPerDay=0.03f;
 UFUNCTION(BlueprintCallable,Category="Soil") void SimulateChunk(UChunkState* Chunk,float DeltaDays,float Temperature);
 UFUNCTION(BlueprintCallable,Category="Soil") void ApplyIrrigation(UChunkState* Chunk,int32 CellIndex,float WaterMm);
};
