#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArrowActor.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API AArrowActor:public AActor{GENERATED_BODY() public: AArrowActor(); UFUNCTION(BlueprintCallable) void LaunchArrow(const FVector& Direction,float Force); UFUNCTION(BlueprintCallable) void SetStoneTip(bool bStone); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ArrowMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseDamage=20.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bStoneTip=false; };
