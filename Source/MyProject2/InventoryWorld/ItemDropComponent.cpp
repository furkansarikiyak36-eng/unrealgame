#include "ItemDropComponent.h"
#include "WorldItemActor.h"
#include "InventoryCore/InventoryComponent.h"
#include "InventoryCore/InventoryItem.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
UItemDropComponent::UItemDropComponent(){PrimaryComponentTick.bCanEverTick=false;}
UInventoryComponent*UItemDropComponent::GetInventory()const{return GetOwner()?GetOwner()->FindComponentByClass<UInventoryComponent>():nullptr;}
bool UItemDropComponent::DropItem(FName ID,int32 Count){UInventoryComponent*Inv=GetInventory();UInventoryItemData*Data=UInventoryItemLibrary::FindItemData(ID);if(!Inv||!Data||Count<=0||!Data->bCanBeDropped||!Inv->HasItem(ID,Count))return false;FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;const FVector Loc=GetOwner()->GetActorLocation()+GetOwner()->GetActorForwardVector()*DropForwardDistance;AWorldItemActor*World=GetWorld()->SpawnActor<AWorldItemActor>(AWorldItemActor::StaticClass(),Loc,FRotator::ZeroRotator,P);if(!World)return false;World->InitializeFromItemData(Data,Count);if(Inv->RemoveItem(ID,Count)!=EInventoryResult::Success){World->Destroy();return false;}if(UPrimitiveComponent*Prim=Cast<UPrimitiveComponent>(World->GetRootComponent()))Prim->AddImpulse(GetOwner()->GetActorForwardVector()*DropImpulse);return true;}
bool UItemDropComponent::DropAllOfItem(FName ID){UInventoryComponent*Inv=GetInventory();return Inv&&DropItem(ID,Inv->GetItemCount(ID));}
