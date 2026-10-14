#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "WaterLayer.generated.h"
UCLASS(BlueprintType)
class MYPROJECT2_API UWaterLayer : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(BlueprintReadWrite) TMap<int32,FWaterCell> Cells;
 UPROPERTY(BlueprintReadWrite) float TotalVolume=0.0f;
 UPROPERTY(BlueprintReadWrite) float AverageLevel=0.0f;
 UPROPERTY(BlueprintReadWrite) bool bIsActive=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 CellsPerAxis=16;
 UFUNCTION(BlueprintCallable,Category="Water") void SetCell(int32 CellKey,const FWaterCell& Cell);
 UFUNCTION(BlueprintPure,Category="Water") FWaterCell GetCell(int32 CellKey) const;
 UFUNCTION(BlueprintCallable,Category="Water") void RecomputeTotals();
};
