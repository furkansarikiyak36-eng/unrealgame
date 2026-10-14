#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AnimalPen.generated.h"
class UStaticMeshComponent; class USphereComponent; class UBoxComponent; class UPointLightComponent; class UParticleSystemComponent; class UPhysicsConstraintComponent; class USplineComponent; class AStickActor;
UCLASS() class MYPROJECT2_API AAnimalPen:public AActor{GENERATED_BODY() public: AAnimalPen(); UFUNCTION(BlueprintCallable) bool CaptureAnimal(AActor* Animal); UFUNCTION(BlueprintCallable) AActor* ReleaseAnimal(int32 Index); UFUNCTION(BlueprintPure) int32 GetAnimalCount()const{return CapturedAnimals.Num();} UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PenMesh; UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> CaptureZone; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxAnimals=4; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TArray<TObjectPtr<AActor>> CapturedAnimals; };
