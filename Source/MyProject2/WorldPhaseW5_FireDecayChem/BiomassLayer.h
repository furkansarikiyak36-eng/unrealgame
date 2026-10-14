#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "BiomassLayer.generated.h"
UCLASS()
class MYPROJECT2_API UBiomassLayer : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(BlueprintReadWrite) TMap<int32,FBiomassCell> Cells;
 UPROPERTY(BlueprintReadWrite) float TotalBiomass=0.0f;
 UFUNCTION(BlueprintCallable,Category="Biomass") void SetCell(int32 CellKey,const FBiomassCell& Cell);
 UFUNCTION(BlueprintPure,Category="Biomass") FBiomassCell GetCell(int32 CellKey) const;
 UFUNCTION(BlueprintCallable,Category="Biomass") void RecomputeTotal();
};
