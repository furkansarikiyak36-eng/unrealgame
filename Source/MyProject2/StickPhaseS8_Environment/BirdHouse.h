#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BirdHouse.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API ABirdHouse:public AActor{GENERATED_BODY() public: ABirdHouse(); UFUNCTION(BlueprintCallable) void AttachToTreeOrWall(AActor* Host); UFUNCTION(BlueprintCallable) void SpawnBird(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HouseMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) float AttractRadius=1500.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float SpawnInterval=60.f; };
