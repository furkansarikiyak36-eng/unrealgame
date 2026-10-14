#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "TargetScorer.generated.h"
UCLASS() class MYPROJECT2_API UTargetScorer : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) float WeightDistance=1.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float WeightVisibility=.5f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float WeightThreat=2.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float WeightAccessibility=1.f; UFUNCTION(BlueprintCallable) float ScoreTarget(AActor* Source,AActor* Target) const; UFUNCTION(BlueprintCallable) AActor* PickBestTarget(AActor* Source,const TArray<AActor*>& Candidates) const; };
