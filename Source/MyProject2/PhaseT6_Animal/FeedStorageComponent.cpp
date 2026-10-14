#include "FeedStorageComponent.h"
UFeedStorageComponent::UFeedStorageComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UFeedStorageComponent::ConsumeFeed(FName ID,float A){if(ID.IsNone()||A<0.f)return false;float* X=FeedStock.Find(ID);if(!X||*X+KINDA_SMALL_NUMBER<A)return false;*X-=A;if(*X<=.01f)FeedStock.Remove(ID);return true;}
void UFeedStorageComponent::AddFeed(FName ID,float A,const FFeedItem& F){if(ID.IsNone()||A<=0.f)return;FeedStock.FindOrAdd(ID)+=A;FeedDefinitions.Add(ID,F);}
float UFeedStorageComponent::GetFeedAmount(FName ID)const{const float* X=FeedStock.Find(ID);return X?*X:0.f;}
bool UFeedStorageComponent::GetFeedDefinition(FName ID,FFeedItem& Out)const{const FFeedItem* F=FeedDefinitions.Find(ID);if(!F)return false;Out=*F;return true;}
