#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WoolProductionComponent.generated.h"
class UAnimalDefinition;
UCLASS(ClassGroup=(Production),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UWoolProductionComponent:public UActorComponent{GENERATED_BODY()
public: UWoolProductionComponent(); UFUNCTION(BlueprintCallable,Category="Wool") void AccumulateDaily(float NutritionFactor,float HealthFactor,float SeasonFactor); UFUNCTION(BlueprintCallable,Category="Wool") float ShearWool(); UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UAnimalDefinition> Definition=nullptr; UPROPERTY(EditAnywhere,BlueprintReadWrite) float DaysBetweenShearing=180.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float AccumulatedWool=0.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float DaysSinceShearing=0.f;};
