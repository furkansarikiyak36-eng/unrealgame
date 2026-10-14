#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimalProductionComponent.generated.h"
class UAnimalDefinition;
UCLASS(ClassGroup=(Production),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UAnimalProductionComponent:public UActorComponent{GENERATED_BODY()
public: UAnimalProductionComponent(); UFUNCTION(BlueprintCallable,Category="Production") float ComputeDailyOutput(float MaxOutput,float AgeFactor,float NutritionFactor,float HealthFactor,float SeasonFactor); UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UAnimalDefinition> Definition=nullptr; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float StoredProduct=0.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxStorage=100.f;};
