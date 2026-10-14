#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeedCompetitionComponent.generated.h"
UCLASS(ClassGroup=(Stress),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UWeedCompetitionComponent:public UActorComponent{GENERATED_BODY()
public: UWeedCompetitionComponent(); UFUNCTION(BlueprintCallable,Category="Weed") void UpdateWeedBiomass(float Growth,float Removal); UFUNCTION(BlueprintPure,Category="Weed") float ComputeYieldMultiplier()const; UFUNCTION(BlueprintPure,Category="Weed") float GetWeedBiomass()const{return WeedBiomass;} UPROPERTY(EditAnywhere,BlueprintReadWrite) float CompetitionAlpha=.7f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MinYieldMultiplier=.2f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxWeedBiomass=500.f;
protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float WeedBiomass=0.f;};
