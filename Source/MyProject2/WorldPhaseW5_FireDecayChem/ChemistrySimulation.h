#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ChemistrySimulation.generated.h"
class UChunkState;
UCLASS()
class MYPROJECT2_API UChemistrySimulation : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float DiffusionRate=0.01f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float DecayRate=0.005f;
 UFUNCTION(BlueprintCallable,Category="Chem") void SimulateChunk(UChunkState* Chunk,float DeltaDays);
 UFUNCTION(BlueprintCallable,Category="Chem") void ApplyToxin(UChunkState* Chunk,int32 CellIndex,float Amount);
};
