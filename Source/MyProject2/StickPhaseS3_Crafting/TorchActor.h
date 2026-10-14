#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TorchActor.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
class UTorchComponent;
UCLASS() class MYPROJECT2_API ATorchActor:public AActor{GENERATED_BODY() public: ATorchActor(); UFUNCTION(BlueprintCallable) void Ignite(); UFUNCTION(BlueprintCallable) void Extinguish(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TorchMesh; UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> TorchLight; UPROPERTY(VisibleAnywhere) TObjectPtr<UTorchComponent> TorchComponent; };
