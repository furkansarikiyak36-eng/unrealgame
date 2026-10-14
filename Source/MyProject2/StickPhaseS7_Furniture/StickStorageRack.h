#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickStorageRack.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API AStickStorageRack:public AActor{GENERATED_BODY() public: AStickStorageRack(); UFUNCTION(BlueprintCallable) bool Store(AStickActor* Stick); UFUNCTION(BlueprintCallable) AStickActor* Retrieve(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RackMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Capacity=60; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TArray<TObjectPtr<AStickActor>> Stored; };
