#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StandingFireActor.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API AStandingFireActor:public AActor{GENERATED_BODY() public: AStandingFireActor(); UFUNCTION(BlueprintCallable) bool Ignite(); UFUNCTION(BlueprintCallable) void Extinguish(); UFUNCTION(BlueprintCallable) bool AddFuel(float Amount); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FireMesh; UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> FireLight; UPROPERTY(VisibleAnywhere) TObjectPtr<UParticleSystemComponent> FireParticles; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float Fuel=0.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxFuel=60.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bIsLit=false; virtual void Tick(float DeltaTime)override; };
