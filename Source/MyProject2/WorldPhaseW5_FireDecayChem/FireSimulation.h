#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WorldTypes.h"
#include "FireSimulation.generated.h"
class UChunkState;
UCLASS()
class MYPROJECT2_API UFireSimulation : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseSpreadRate=0.05f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float HeatPerCell=100.0f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float FireDurationDays=0.5f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float MinBiomassToBurn=5.0f;
 UFUNCTION(BlueprintCallable,Category="Fire") void SimulateChunk(UChunkState* Chunk,float DeltaDays,float WindSpeed,FVector2D WindDir);
 UFUNCTION(BlueprintCallable,Category="Fire") bool TryIgnite(UChunkState* Chunk,int32 CellIndex);
 UFUNCTION(BlueprintCallable,Category="Fire") void ExtinguishCell(UChunkState* Chunk,int32 CellIndex);
protected: void SpreadFire(UChunkState* Chunk,int32 SourceIdx,float DeltaDays,FVector2D WindDir); void BroadcastFireEvent(UChunkState* Chunk,EWorldEventType Type,float Magnitude,int32 CellIndex);
};
