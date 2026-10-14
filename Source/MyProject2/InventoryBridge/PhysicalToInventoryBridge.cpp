#include "PhysicalToInventoryBridge.h"
#include "InventoryCore/InventoryComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StickPhaseS1_Core/StickActor.h"
UPhysicalToInventoryBridge::UPhysicalToInventoryBridge(){PrimaryComponentTick.bCanEverTick=false;}
UInventoryComponent*UPhysicalToInventoryBridge::GetInventory()const{return GetOwner()?GetOwner()->FindComponentByClass<UInventoryComponent>():nullptr;}
#define CONVERT_PHYSICAL_TO_ITEM(Method,Type) bool UPhysicalToInventoryBridge::Method(Type*Obj,FName ID){UInventoryComponent*Inv=GetInventory();if(!Inv||!Obj||ID.IsNone())return false;if(Inv->AddItem(ID,1)!=EInventoryResult::Success)return false;if(bDestroyPhysicalAfterConvert)Obj->Destroy();return true;}
CONVERT_PHYSICAL_TO_ITEM(ConvertLogToItem,ALogActor)
CONVERT_PHYSICAL_TO_ITEM(ConvertStoneToItem,AStoneActor)
CONVERT_PHYSICAL_TO_ITEM(ConvertStickToItem,AStickActor)
#undef CONVERT_PHYSICAL_TO_ITEM
