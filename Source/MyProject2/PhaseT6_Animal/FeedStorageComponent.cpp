#include "FeedStorageComponent.h"
UFeedStorageComponent::UFeedStorageComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UFeedStorageComponent::ConsumeFeed(FName ID,float A){if(ID.IsNone()||A<0.f)return false;float* X=FeedStock.Find(ID);if(!X||*X+KINDA_SMALL_NUMBER<A)return false;*X-=A;if(*X<=.01f)FeedStock.Remove(ID);return true;}
void UFeedStorageComponent::AddFeed(FName ID,float A,const FFeedItem& F){TryAddFeed(ID,A,F);}
float UFeedStorageComponent::GetTotalFeedAmount()const{float Total=0.f;for(const auto& Pair:FeedStock)if(FMath::IsFinite(Pair.Value)&&Pair.Value>0.f)Total+=Pair.Value;return Total;}
bool UFeedStorageComponent::CanAddFeed(FName ID,float A)const
{
 if(ID.IsNone()||!FMath::IsFinite(A)||A<=0.f||!FMath::IsFinite(StorageCapacity)||StorageCapacity<0.f)return false;
 const float Existing=GetFeedAmount(ID);
 return FMath::IsFinite(Existing)&&Existing>=0.f&&GetTotalFeedAmount()+A<=StorageCapacity+KINDA_SMALL_NUMBER;
}
bool UFeedStorageComponent::TryAddFeed(FName ID,float A,const FFeedItem& F)
{
 if(F.FeedID!=ID||!FMath::IsFinite(F.DryMatter)||!FMath::IsFinite(F.MetabolizableEnergy)||!FMath::IsFinite(F.CrudeProtein)||
    F.DryMatter<0.f||F.DryMatter>1.f||F.MetabolizableEnergy<0.f||F.CrudeProtein<0.f||F.CrudeProtein>1.f||!CanAddFeed(ID,A))return false;
 FeedStock.FindOrAdd(ID)+=A;FeedDefinitions.Add(ID,F);return true;
}
float UFeedStorageComponent::GetFeedAmount(FName ID)const{const float* X=FeedStock.Find(ID);return X?*X:0.f;}
bool UFeedStorageComponent::GetFeedDefinition(FName ID,FFeedItem& Out)const{const FFeedItem* F=FeedDefinitions.Find(ID);if(!F)return false;Out=*F;return true;}
