#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StakeActor.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API AStakeActor:public AActor{GENERATED_BODY() public: AStakeActor(); UFUNCTION(BlueprintCallable) void Sharpen(); UFUNCTION(BlueprintCallable) void ApplyDamageToTarget(AActor* Target,float DamageAmount); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> StakeMesh; UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> DamageSphere; UPROPERTY(EditAnywhere,BlueprintReadWrite) float Damage=35.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bSharpened=false; };
