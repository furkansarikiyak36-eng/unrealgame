#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QualityComponent.generated.h"
UCLASS(ClassGroup=(Farming),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UQualityComponent:public UActorComponent{GENERATED_BODY()
public:
 UQualityComponent();
 UFUNCTION(BlueprintCallable,Category="QualityComponent") float ComputeQuality(float BaseQuality,float DaysPastMaturity,float DamageFactor) const;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float DecayRatePerDay=.02f;
};