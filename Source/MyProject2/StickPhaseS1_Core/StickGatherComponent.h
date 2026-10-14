#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickGatherComponent.generated.h"
class AStickActor; class ASaplingActor;
UCLASS(ClassGroup=(Stick), meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStickGatherComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStickGatherComponent(); UFUNCTION(BlueprintCallable) bool GatherFromGround(AStickActor* Stick); UFUNCTION(BlueprintCallable) bool HarvestSapling(ASaplingActor* Sapling,float Strength); UFUNCTION(BlueprintCallable) int32 GatherNearbySticks(float Radius);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float GatherRadius=250.f; UPROPERTY(EditAnywhere, BlueprintReadWrite) float GatherStrength=1.f; UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AutoGatherBatch=5;
};
