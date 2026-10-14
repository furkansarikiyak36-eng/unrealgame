#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickChair.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API AStickChair:public AActor{GENERATED_BODY() public: AStickChair(); UFUNCTION(BlueprintCallable) bool Sit(AActor* Sitter); UFUNCTION(BlueprintCallable) void Stand(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ChairMesh; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bOccupied=false; };
