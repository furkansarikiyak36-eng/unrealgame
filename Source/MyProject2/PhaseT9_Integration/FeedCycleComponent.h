#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FeedCycleComponent.generated.h"
UCLASS(ClassGroup=(Farming),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UFeedCycleComponent:public UActorComponent{GENERATED_BODY()
public:
 UFeedCycleComponent();
 UFUNCTION(BlueprintCallable,Category="FeedCycleComponent") void AddHarvestedFeed(FName FeedID,float Amount);
 UFUNCTION(BlueprintCallable,Category="FeedCycleComponent") float ConsumeForAnimals(FName FeedID,float Amount);
 UFUNCTION(BlueprintCallable,Category="FeedCycleComponent") void UpdateDaily(float DailyConsumption,float DailyProduction);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float StorageCapacity=1000.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TMap<FName,float> FeedStock;
};