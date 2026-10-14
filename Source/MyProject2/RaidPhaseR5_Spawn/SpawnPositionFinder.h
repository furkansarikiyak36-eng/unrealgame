#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SpawnPositionFinder.generated.h"
UCLASS() class MYPROJECT2_API USpawnPositionFinder : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) float MinDistance=1500.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxDistance=4000.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float ClearanceRadius=60.f; FVector FindValidSpawnPosition(UWorld* World,const FVector& TargetLocation); bool IsPositionValid(UWorld* World,const FVector& Position) const; bool HasClearance(UWorld* World,const FVector& Position) const; bool IsOnNavMesh(UWorld* World,const FVector& Position) const; };
