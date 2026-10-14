#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickBed.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API AStickBed:public AActor{GENERATED_BODY() public: AStickBed(); UFUNCTION(BlueprintCallable) bool Sleep(AActor* Sleeper); UFUNCTION(BlueprintCallable) void WakeUp(); UFUNCTION(BlueprintPure) bool IsOccupied()const{return bOccupied;} UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BedMesh; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bOccupied=false; UPROPERTY(EditAnywhere,BlueprintReadWrite) float ComfortLevel=.5f; };
