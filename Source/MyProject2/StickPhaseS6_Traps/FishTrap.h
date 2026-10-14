#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FishTrap.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
class UTrapMechanismComponent;
UCLASS() class MYPROJECT2_API AFishTrap:public AActor{GENERATED_BODY() public: AFishTrap(); UFUNCTION(BlueprintCallable) void PlaceInWater(const FVector& Location); UFUNCTION(BlueprintCallable) int32 CollectFish(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TrapMesh; UPROPERTY(VisibleAnywhere) TObjectPtr<UTrapMechanismComponent> Mechanism; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) int32 StoredFish=0; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxFish=5; UPROPERTY(EditAnywhere,BlueprintReadWrite) float CatchRate=.1f; virtual void Tick(float DeltaTime)override; };
