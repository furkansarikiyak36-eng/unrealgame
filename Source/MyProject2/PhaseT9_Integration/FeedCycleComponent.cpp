#include "FeedCycleComponent.h"
#include "PhaseT6_Animal/FeedStorageComponent.h"
#include "GameFramework/Actor.h"

UFeedCycleComponent::UFeedCycleComponent(){PrimaryComponentTick.bCanEverTick=false;}

UFeedStorageComponent* UFeedCycleComponent::ResolveFeedStorage(bool) 
{
 AActor* Owner=GetOwner();
 return Owner?Owner->FindComponentByClass<UFeedStorageComponent>():nullptr;
}

void UFeedCycleComponent::AddHarvestedFeed(FName FeedID,float Amount)
{
 if(FeedID.IsNone()||!FMath::IsFinite(Amount)||Amount<=0.0f)return;
 UFeedStorageComponent* Storage=ResolveFeedStorage(false);
 if(!Storage||Storage->GetTotalFeedAmount()+Amount>FMath::Min(StorageCapacity,Storage->StorageCapacity)+KINDA_SMALL_NUMBER)return;
 FFeedItem Feed;Feed.FeedID=FeedID;Feed.DryMatter=0.85f;Feed.MetabolizableEnergy=10.0f;Feed.CrudeProtein=0.15f;
 Storage->TryAddFeed(FeedID,Amount,Feed);
}

float UFeedCycleComponent::ConsumeForAnimals(FName FeedID,float Amount)
{
 if(FeedID.IsNone()||!FMath::IsFinite(Amount)||Amount<=0.0f)return 0.0f;
 UFeedStorageComponent* Storage=ResolveFeedStorage(false);
 if(!Storage)return 0.0f;
 const float Taken=FMath::Min(Amount,Storage->GetFeedAmount(FeedID));
 return Taken>KINDA_SMALL_NUMBER&&Storage->ConsumeFeed(FeedID,Taken)?Taken:0.0f;
}

void UFeedCycleComponent::UpdateDaily(float DailyConsumption,float DailyProduction)
{
 UpdateDailyForFeed(NAME_None,DailyConsumption,DailyProduction);
}

void UFeedCycleComponent::UpdateDailyForFeed(FName FeedID,float DailyConsumption,float DailyProduction)
{
 UFeedStorageComponent* Storage=ResolveFeedStorage(false);
 if(!Storage)return;
 if(!FeedID.IsNone()&&FMath::IsFinite(DailyProduction)&&DailyProduction>0.0f)
 {
   FFeedItem Feed;Feed.FeedID=FeedID;Feed.DryMatter=0.85f;Feed.MetabolizableEnergy=10.0f;Feed.CrudeProtein=0.15f;
   const float Capacity=FMath::Max(0.0f,FMath::Min(StorageCapacity,Storage->StorageCapacity));
   const float Room=FMath::Max(0.0f,Capacity-Storage->GetTotalFeedAmount());
   if(DailyProduction<=Room+KINDA_SMALL_NUMBER)Storage->TryAddFeed(FeedID,DailyProduction,Feed);
 }
 float Remaining=FMath::IsFinite(DailyConsumption)?FMath::Max(0.0f,DailyConsumption):0.0f;
 TArray<FName> FeedIDs;
 for(const TPair<FName,float>& Pair:Storage->FeedStock)if(Pair.Value>0.0f)FeedIDs.Add(Pair.Key);
 FeedIDs.Sort([](const FName& A,const FName& B){return A.ToString()<B.ToString();});
 for(const FName ID:FeedIDs)
 {
   if(Remaining<=KINDA_SMALL_NUMBER)break;
   const float Take=FMath::Min(Storage->GetFeedAmount(ID),Remaining);
   if(Take>KINDA_SMALL_NUMBER&&Storage->ConsumeFeed(ID,Take))Remaining-=Take;
 }
}
