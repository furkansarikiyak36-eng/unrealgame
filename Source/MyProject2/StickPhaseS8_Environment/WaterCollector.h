#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaterCollector.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API AWaterCollector:public AActor{GENERATED_BODY() public: AWaterCollector(); UFUNCTION(BlueprintCallable) float CollectWater(float Amount); UFUNCTION(BlueprintCallable) bool AddContainer(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FrameMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxWater=100.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float CurrentWater=0.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float RainCollectionRate=.5f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bHasContainer=false; virtual void Tick(float DeltaTime)override; };
