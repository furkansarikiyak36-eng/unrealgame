#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SmallAnimalTrap.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
class UTrapMechanismComponent;
UCLASS() class MYPROJECT2_API ASmallAnimalTrap:public AActor{GENERATED_BODY() public: ASmallAnimalTrap(); UFUNCTION(BlueprintCallable) bool CatchAnimal(AActor* Animal); UFUNCTION(BlueprintCallable) AActor* ReleaseAnimal(); UFUNCTION(BlueprintCallable) void ResetTrap(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TrapMesh; UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> TriggerVolume; UPROPERTY(VisibleAnywhere) TObjectPtr<UTrapMechanismComponent> Mechanism; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TObjectPtr<AActor> CaughtAnimal=nullptr; };
