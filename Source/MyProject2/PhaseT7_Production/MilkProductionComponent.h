#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MilkProductionComponent.generated.h"
class UAnimalDefinition;
UCLASS(ClassGroup=(Production),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UMilkProductionComponent:public UActorComponent{GENERATED_BODY()
public: UMilkProductionComponent(); UFUNCTION(BlueprintPure,Category="Milk") float ComputePotentialMilk(float LactationDay)const; UFUNCTION(BlueprintCallable,Category="Milk") float ComputeActualMilk(float Potential,float EnergyFactor,float ProteinFactor,float HealthFactor,float TempFactor); UFUNCTION(BlueprintCallable,Category="Milk") float Milking(float Amount); UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UAnimalDefinition> Definition=nullptr; UPROPERTY(EditAnywhere,BlueprintReadWrite) float LactationCurveA=.15f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float LactationCurveB=.3f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float LactationCurveC=.005f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float StoredMilk=0.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxStoredMilk=100.f;};
