#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "ChemistryLayer.generated.h"
UCLASS()
class MYPROJECT2_API UChemistryLayer : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(BlueprintReadWrite) TMap<int32,FChemistryCell> Cells;
 UFUNCTION(BlueprintCallable,Category="Chem") void SetToxin(int32 CellKey,float Concentration);
 UFUNCTION(BlueprintPure,Category="Chem") float GetToxin(int32 CellKey) const;
 UFUNCTION(BlueprintCallable,Category="Chem") void RemoveCleanCells(float Threshold=0.001f);
};
