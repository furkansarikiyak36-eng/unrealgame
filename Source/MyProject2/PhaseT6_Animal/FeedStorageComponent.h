#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimalNutritionComponent.h"
#include "FeedStorageComponent.generated.h"
UCLASS(ClassGroup=(Animal),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UFeedStorageComponent:public UActorComponent{GENERATED_BODY()
public: UFeedStorageComponent(); UFUNCTION(BlueprintCallable,Category="Feed") bool ConsumeFeed(FName FeedID,float Amount); UFUNCTION(BlueprintCallable,Category="Feed") void AddFeed(FName FeedID,float Amount,const FFeedItem& FeedInfo);
 UFUNCTION(BlueprintPure,Category="Feed") bool CanAddFeed(FName FeedID,float Amount)const;
 UFUNCTION(BlueprintCallable,Category="Feed") bool TryAddFeed(FName FeedID,float Amount,const FFeedItem& FeedInfo);
 UFUNCTION(BlueprintPure,Category="Feed") float GetTotalFeedAmount()const; UFUNCTION(BlueprintPure,Category="Feed") float GetFeedAmount(FName FeedID)const; UFUNCTION(BlueprintCallable,Category="Feed") bool GetFeedDefinition(FName FeedID,FFeedItem& OutFeed)const; UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Capacity") float StorageCapacity=1000.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TMap<FName,float> FeedStock; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TMap<FName,FFeedItem> FeedDefinitions;};
