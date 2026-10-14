#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "ActionCostComparator.generated.h"
UCLASS() class MYPROJECT2_API UActionCostComparator : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) float MovementWeight=1.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float DelayWeight=.5f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float RiskWeight=.8f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float AttackWeight=1.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float TimeWeight=.7f; UFUNCTION(BlueprintCallable) float ComputeRouteCost(const FVector& Start,const FVector& Goal) const; UFUNCTION(BlueprintCallable) float ComputeBreakCost(AActor* Obstacle) const; UFUNCTION(BlueprintCallable) EAlternativeAction ChooseCheaperAction(const FVector& Start,const FVector& Goal,AActor* Obstacle) const; };
