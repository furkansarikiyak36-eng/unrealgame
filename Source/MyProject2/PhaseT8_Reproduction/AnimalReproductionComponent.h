#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimalReproductionComponent.generated.h"
class UAnimalDefinition;
UCLASS(ClassGroup=(Reproduction),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UAnimalReproductionComponent:public UActorComponent{GENERATED_BODY()
public: UAnimalReproductionComponent(); UFUNCTION(BlueprintPure,Category="Reproduction") bool IsEligible(float Age,float Health,float BCS,int32 FreeCapacity)const; UFUNCTION(BlueprintPure,Category="Reproduction") float ComputeConceptionProbability(float Health,float BCS,float Nutrition)const; UFUNCTION(BlueprintCallable,Category="Reproduction") void UpdateReproduction(float Age,float Health,float BCS,float Nutrition,float DeltaDays,int32 Seed); UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UAnimalDefinition> Definition=nullptr; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MinHealth=60.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MinBCS=2.5f;
protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float ReproductionProgress=0.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float ConceptionThreshold=10.f; uint32 RollSequence=0;
};
