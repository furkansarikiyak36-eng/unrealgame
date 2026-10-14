#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BowActor.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
class AArrowActor;
UCLASS() class MYPROJECT2_API ABowActor:public AActor{GENERATED_BODY() public: ABowActor(); UFUNCTION(BlueprintCallable) void DrawBow(float DrawAmount); UFUNCTION(BlueprintCallable) void ReleaseBow(); UFUNCTION(BlueprintCallable) void NockArrow(AArrowActor* Arrow); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BowMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxDrawForce=800.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float CurrentDraw=0.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TObjectPtr<AArrowActor> NockedArrow=nullptr; };
