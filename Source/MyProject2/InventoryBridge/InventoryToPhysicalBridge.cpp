#include "InventoryToPhysicalBridge.h"
#include "InventoryCore/InventoryComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "Engine/World.h"
UInventoryToPhysicalBridge::UInventoryToPhysicalBridge(){PrimaryComponentTick.bCanEverTick=false;}
UInventoryComponent*UInventoryToPhysicalBridge::GetInventory()const{return GetOwner()?GetOwner()->FindComponentByClass<UInventoryComponent>():nullptr;}
template<typename T>static T*SpawnAfterInventoryCheck(UInventoryToPhysicalBridge*Self,FName ID,const FVector&Loc,TSubclassOf<T>Class){if(!Self||!Class)return nullptr;UInventoryComponent*Inv=Self->GetOwner()?Self->GetOwner()->FindComponentByClass<UInventoryComponent>():nullptr;if(!Inv||!Inv->HasItem(ID,1))return nullptr;FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;T*Obj=Self->GetWorld()->SpawnActor<T>(Class,Loc,FRotator::ZeroRotator,P);if(!Obj)return nullptr;if(Inv->RemoveItem(ID,1)!=EInventoryResult::Success){Obj->Destroy();return nullptr;}return Obj;}
ALogActor*UInventoryToPhysicalBridge::SpawnLogFromInventory(FName ID,const FVector&L){return SpawnAfterInventoryCheck(this,ID,L,LogClass);}
AStoneActor*UInventoryToPhysicalBridge::SpawnStoneFromInventory(FName ID,const FVector&L){return SpawnAfterInventoryCheck(this,ID,L,StoneClass);}
AStickActor*UInventoryToPhysicalBridge::SpawnStickFromInventory(FName ID,const FVector&L){return SpawnAfterInventoryCheck(this,ID,L,StickClass);}
