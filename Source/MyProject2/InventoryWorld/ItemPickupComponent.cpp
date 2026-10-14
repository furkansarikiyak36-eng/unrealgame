#include "ItemPickupComponent.h"
#include "WorldItemActor.h"
#include "InventoryCore/InventoryComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
UItemPickupComponent::UItemPickupComponent(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=0.25f;}
UInventoryComponent* UItemPickupComponent::GetInventory()const{return GetOwner()?GetOwner()->FindComponentByClass<UInventoryComponent>():nullptr;}
bool UItemPickupComponent::PickupItem(AWorldItemActor* Item){if(!Item||!Item->IsPickupable())return false;UInventoryComponent*Inv=GetInventory();if(!Inv)return false;const EInventoryResult R=Inv->AddItemData(Item->GetItemData());if(R!=EInventoryResult::Success)return false;Item->Destroy();return true;}
int32 UItemPickupComponent::PickupNearbyItems(float Radius){int32 N=0;for(TActorIterator<AWorldItemActor>It(GetWorld());It;++It){AWorldItemActor*I=*It;if(I&&I->IsPickupable()&&FVector::Dist(GetOwner()->GetActorLocation(),I->GetActorLocation())<=Radius&&PickupItem(I))++N;}return N;}
AWorldItemActor* UItemPickupComponent::FindNearestPickupable(float R)const{AWorldItemActor*Best=nullptr;float DBest=R;for(TActorIterator<AWorldItemActor>It(GetWorld());It;++It){AWorldItemActor*I=*It;if(!I||!I->IsPickupable())continue;float D=FVector::Dist(GetOwner()->GetActorLocation(),I->GetActorLocation());if(D<DBest){DBest=D;Best=I;}}return Best;}
void UItemPickupComponent::TickComponent(float D,ELevelTick T,FActorComponentTickFunction*F){Super::TickComponent(D,T,F);if(bAutoPickup)PickupNearbyItems(AutoPickupRadius);}
