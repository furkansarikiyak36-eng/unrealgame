#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimalEnergyComponent.generated.h"
class UAnimalDefinition;
UCLASS(ClassGroup=(Animal),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UAnimalEnergyComponent:public UActorComponent{GENERATED_BODY()
public: UAnimalEnergyComponent(); UFUNCTION(BlueprintPure,Category="Energy") float ComputeRequirements(float Weight,bool bGrowing,bool bLactating,bool bPregnant,float TempDeviation)const; UFUNCTION(BlueprintCallable,Category="Energy") void UpdateEnergyBalance(float Intake,float Requirement,float DeltaDays); UFUNCTION(BlueprintPure,Category="Energy") float GetBCS()const{return BodyConditionScore;} UFUNCTION(BlueprintPure,Category="Energy") float GetEnergyBalance()const{return EnergyBalance;} UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UAnimalDefinition> Definition=nullptr; UPROPERTY(EditAnywhere,BlueprintReadWrite) float BCS_Min=1.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float BCS_Max=5.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float k_B=.01f;
protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float BodyConditionScore=3.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float EnergyBalance=0.f;};
