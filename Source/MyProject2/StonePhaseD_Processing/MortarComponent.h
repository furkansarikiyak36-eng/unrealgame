#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MortarComponent.generated.h"
class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UMortarComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UMortarComponent();
    UFUNCTION(BlueprintCallable) bool PrepareMortar(int32 SandAmount, int32 LimeAmount, int32 WaterAmount);
    UFUNCTION(BlueprintCallable) bool ApplyMortar(AStoneActor* StoneA, AStoneActor* StoneB);
    UFUNCTION(BlueprintCallable) bool FillJoint(AStoneActor* Stone);
    UFUNCTION(BlueprintPure) float GetMortarAmount() const { return MortarAmount; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MortarAmount = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MortarStrengthMultiplier = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float WaterDissolveRate = 0.01f;
};
