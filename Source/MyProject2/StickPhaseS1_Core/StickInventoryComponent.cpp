#include "StickInventoryComponent.h"
UStickInventoryComponent::UStickInventoryComponent(){PrimaryComponentTick.bCanEverTick=false;}
int32 UStickInventoryComponent::AddSticks(FName Type,int32 Count)
{
    if(Type.IsNone()||Count<=0)return 0; int32 Total=0;for(const FStickBundle& B:Bundles)Total+=B.Count;int32 Remaining=FMath::Min(Count,FMath::Max(0,GlobalCapacity-Total));const int32 Accepted=Remaining;
    for(FStickBundle& B:Bundles){if(B.StickType!=Type||Remaining<=0)continue;const int32 Add=FMath::Min(Remaining,FMath::Max(0,B.MaxCount-B.Count));B.Count+=Add;Remaining-=Add;}
    while(Remaining>0){FStickBundle B;B.StickType=Type;B.Count=FMath::Min(Remaining,20);B.MaxCount=20;Bundles.Add(B);Remaining-=B.Count;}
    return Accepted;
}
int32 UStickInventoryComponent::RemoveSticks(FName Type,int32 Count){if(Count<=0)return 0;int32 Removed=0;for(int32 i=Bundles.Num()-1;i>=0&&Removed<Count;--i)if(Bundles[i].StickType==Type){int32 N=FMath::Min(Bundles[i].Count,Count-Removed);Bundles[i].Count-=N;Removed+=N;if(Bundles[i].Count<=0)Bundles.RemoveAt(i);}return Removed;}
int32 UStickInventoryComponent::GetCount(FName Type)const{int32 N=0;for(const FStickBundle& B:Bundles)if(B.StickType==Type)N+=B.Count;return N;}
