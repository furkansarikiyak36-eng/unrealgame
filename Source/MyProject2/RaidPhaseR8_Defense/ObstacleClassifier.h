#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "ObstacleClassifier.generated.h"
UCLASS() class MYPROJECT2_API UObstacleClassifier : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) EObstacleType ClassifyObstacle(AActor* Obstacle) const; };
