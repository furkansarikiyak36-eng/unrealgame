#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EggProductionComponent.generated.h"
class UAnimalDefinition;
UCLASS(ClassGroup=(Production),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UEggProductionComponent:public UActorComponent{GENERATED_BODY()
public: UEggProductionComponent(); UFUNCTION(BlueprintCallable,Category="Egg") float ComputeDailyEggs(float AgeFactor,float NutritionFactor,float HealthFactor,float SeasonFactor); UFUNCTION(BlueprintCallable,Category="Egg") int32 CollectEggs(); UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UAnimalDefinition> Definition=nullptr; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float EggAccumulator=0.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float EggMaturityAge=150.f;};
