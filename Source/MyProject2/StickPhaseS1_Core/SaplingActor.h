#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "SaplingActor.generated.h"
class UStaticMeshComponent; class AStickActor;
UENUM(BlueprintType) enum class ESaplingType : uint8 { Thin, Thick, Forked, Bush };
UCLASS() class MYPROJECT2_API ASaplingActor : public AActor
{
    GENERATED_BODY()
public:
    ASaplingActor(); UFUNCTION(BlueprintCallable) int32 Harvest(float Strength, AActor* InstigatorActor); UFUNCTION(BlueprintPure) bool IsHarvested() const { return RemainingSticks <= 0; }
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> SaplingMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ESaplingType Type = ESaplingType::Thin;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TotalSticks = 3;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) int32 RemainingSticks = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AStickActor> StickClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RegrowthTime = 120.f;
protected: FTimerHandle RegrowthTimer; UFUNCTION() void OnRegrow(); virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
