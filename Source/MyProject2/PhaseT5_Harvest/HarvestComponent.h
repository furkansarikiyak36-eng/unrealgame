#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HarvestComponent.generated.h"
class UPlantGrowthComponent;
UCLASS(ClassGroup=(Harvest),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UHarvestComponent:public UActorComponent{GENERATED_BODY()
public: UHarvestComponent(); UFUNCTION(BlueprintPure,Category="Harvest") bool CanHarvest()const; UFUNCTION(BlueprintCallable,Category="Harvest") float Harvest(); UPROPERTY(EditAnywhere,BlueprintReadWrite) float BaseYield=400.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float HarvestIndex=.4f;};
