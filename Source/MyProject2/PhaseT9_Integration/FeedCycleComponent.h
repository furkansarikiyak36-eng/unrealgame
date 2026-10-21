#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
class UFeedStorageComponent;
#include "FeedCycleComponent.generated.h"
UCLASS(ClassGroup=(Farming),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UFeedCycleComponent:public UActorComponent{GENERATED_BODY()
public:
 UFeedCycleComponent();
 UFUNCTION(BlueprintCallable,Category="FeedCycleComponent") void AddHarvestedFeed(FName FeedID,float Amount);
 UFUNCTION(BlueprintCallable,Category="FeedCycleComponent") float ConsumeForAnimals(FName FeedID,float Amount);
 UFUNCTION(BlueprintCallable,Category="FeedCycleComponent") void UpdateDaily(float DailyConsumption,float DailyProduction);
 UFUNCTION(BlueprintCallable,Category="FeedCycleComponent") void UpdateDailyForFeed(FName FeedID,float DailyConsumption,float DailyProduction);
protected: UFeedStorageComponent* ResolveFeedStorage(bool bCreateIfMissing);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float StorageCapacity=1000.f;
};