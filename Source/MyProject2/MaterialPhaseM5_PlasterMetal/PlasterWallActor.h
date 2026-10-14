// PlasterWallActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlasterWallActor.generated.h"

class UStaticMeshComponent;
class UPlasterComponent;

UCLASS()
class MYPROJECT2_API APlasterWallActor : public AActor
{
    GENERATED_BODY()

public:
    APlasterWallActor();

    UFUNCTION(BlueprintCallable, Category = "Wall")
    void InitializeWall(float Thickness);

    UFUNCTION(BlueprintCallable, Category = "Wall")
    void UpdatePlaster(float DeltaHours, float WaterIntensity);

    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> WallMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPlasterComponent> PlasterComponent;
};
