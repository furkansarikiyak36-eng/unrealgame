#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Scarecrow.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API AScarecrow:public AActor{GENERATED_BODY() public: AScarecrow(); UFUNCTION(BlueprintCallable) void SetScareRadius(float Radius); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ScarecrowMesh; UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> ScareVolume; UPROPERTY(EditAnywhere,BlueprintReadWrite) float ScareRadius=800.f; };
