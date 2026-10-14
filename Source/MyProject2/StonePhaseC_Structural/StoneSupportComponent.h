#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneSupportComponent.generated.h"
class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneSupportComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneSupportComponent();
    UFUNCTION(BlueprintCallable) bool CanRemoveColumn(AStoneActor* Column) const;
    UFUNCTION(BlueprintCallable) TArray<AStoneActor*> GetOrphansIfRemoved(AStoneActor* Column) const;
    UFUNCTION(BlueprintCallable) bool AddBrace(AStoneActor* Wall, AStoneActor* Brace);
    UFUNCTION(BlueprintCallable) bool IsLaterallyStable(AStoneActor* Wall) const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float LateralStabilityThreshold = 0.4f;
};
