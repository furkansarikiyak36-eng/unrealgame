#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpearActor.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API ASpearActor:public AActor{GENERATED_BODY() public: ASpearActor(); UFUNCTION(BlueprintCallable) void ThrowSpear(const FVector& Direction,float Force); UFUNCTION(BlueprintCallable) void MeleeAttack(AActor* Target,float Damage); UFUNCTION(BlueprintPure) bool IsThrown()const{return bThrown;} UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> SpearMesh; UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> DamageSphere; UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseDamage=25.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float ThrowDamageBonus=40.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bThrown=false; };
