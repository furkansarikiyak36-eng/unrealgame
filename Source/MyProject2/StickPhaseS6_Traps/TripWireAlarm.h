#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TripWireAlarm.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
class UTrapMechanismComponent;
UCLASS() class MYPROJECT2_API ATripWireAlarm:public AActor{GENERATED_BODY() public: ATripWireAlarm(); UFUNCTION(BlueprintCallable) void StretchWire(const FVector& Start,const FVector& End); UFUNCTION(BlueprintCallable) void TriggerAlarm(AActor* InstigatorActor); UPROPERTY(VisibleAnywhere) TObjectPtr<USplineComponent> Wire; UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> TriggerVolume; UPROPERTY(VisibleAnywhere) TObjectPtr<UTrapMechanismComponent> Mechanism; UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> AlarmLight; UPROPERTY(EditAnywhere,BlueprintReadWrite) float AlarmRadius=3000.f; };
