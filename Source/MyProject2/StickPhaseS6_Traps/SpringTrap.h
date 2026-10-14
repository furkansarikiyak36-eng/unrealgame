#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpringTrap.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
class UTrapMechanismComponent;
UCLASS() class MYPROJECT2_API ASpringTrap:public AActor{GENERATED_BODY() public: ASpringTrap(); UFUNCTION(BlueprintCallable) void SetSpringTension(float TensionAmount); UFUNCTION(BlueprintCallable) void ReleaseSpring(AActor* Victim); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BaseMesh; UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> SpringArm; UPROPERTY(VisibleAnywhere) TObjectPtr<UPhysicsConstraintComponent> SpringConstraint; UPROPERTY(VisibleAnywhere) TObjectPtr<UTrapMechanismComponent> Mechanism; UPROPERTY(EditAnywhere,BlueprintReadWrite) float Tension=.5f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float LaunchForce=2000.f; };
