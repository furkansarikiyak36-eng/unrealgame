#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "TerrainLayer.generated.h"
UCLASS()
class MYPROJECT2_API UTerrainLayer : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(BlueprintReadWrite) TMap<int32,FTerrainCell> Cells;
 UFUNCTION(BlueprintCallable,Category="Terrain") void DeformCell(int32 CellKey,float DeltaHeight);
 UFUNCTION(BlueprintPure,Category="Terrain") float GetHeight(int32 CellKey) const;
 UFUNCTION(BlueprintPure,Category="Terrain") bool IsDeformed(int32 CellKey) const;
};
