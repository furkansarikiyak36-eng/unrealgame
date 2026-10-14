#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BiomassComponent.generated.h"
UCLASS(ClassGroup=(Farming),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UBiomassComponent:public UActorComponent{GENERATED_BODY()
public:
 UBiomassComponent();
 UFUNCTION(BlueprintCallable,Category="BiomassComponent") void AccumulateBiomass(float DailyTranspiration,float WaterUseEfficiency,float StressMultiplier);
 UFUNCTION(BlueprintCallable,Category="BiomassComponent") float GetBiomass() const;
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float Biomass=0.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxBiomass=2000.f;
};