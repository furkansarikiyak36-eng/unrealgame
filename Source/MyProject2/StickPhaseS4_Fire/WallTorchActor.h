#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WallTorchActor.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
class UTorchComponent;
UCLASS() class MYPROJECT2_API AWallTorchActor:public AActor{GENERATED_BODY() public: AWallTorchActor(); UFUNCTION(BlueprintCallable) bool AttachToWall(AActor* Wall); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MountMesh; UPROPERTY(VisibleAnywhere) TObjectPtr<UTorchComponent> TorchComponent; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TObjectPtr<AActor> AttachedWall=nullptr; };
