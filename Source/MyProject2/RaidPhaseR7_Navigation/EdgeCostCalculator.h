#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EdgeCostCalculator.generated.h"
UCLASS() class MYPROJECT2_API UEdgeCostCalculator : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) float DangerWeight=.5f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float TerrainWeight=.3f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MovementWeight=.2f; UFUNCTION(BlueprintCallable) float ComputeEdgeCost(const FVector&A,const FVector&B,float DangerValue) const; };
