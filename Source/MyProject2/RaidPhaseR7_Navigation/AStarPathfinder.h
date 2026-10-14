#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AStarPathfinder.generated.h"
class UNavGraph;
UCLASS() class MYPROJECT2_API UAStarPathfinder : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxIterations=2000; UFUNCTION(BlueprintCallable) TArray<FVector> FindPath(UNavGraph* Graph,const FVector& Start,const FVector& Goal) const; };
