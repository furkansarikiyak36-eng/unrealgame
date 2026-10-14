#include "InventorySaveSubsystem.h"
#include "InventoryComponent.h"
#include "QuickAccessComponent.h"
#include "Kismet/GameplayStatics.h"

bool UInventorySaveSubsystem::SaveInventory(UInventoryComponent* Inventory,const FString& SlotName)
{
 if(!Inventory||SlotName.IsEmpty())return false;
 UInventorySaveGame* Save=Cast<UInventorySaveGame>(UGameplayStatics::CreateSaveGameObject(UInventorySaveGame::StaticClass()));if(!Save)return false;
 for(const FInventorySlot&S:Inventory->GetSlots())if(S.IsOccupied())Save->Items.Add(S.Item);
 if(AActor* Owner=Inventory->GetOwner())if(UQuickAccessComponent* QA=Owner->FindComponentByClass<UQuickAccessComponent>()){
  Save->ActiveQuickSlot=QA->GetActiveSlot();for(int32 i=0;i<QA->SlotCount;++i)Save->QuickSlots.Add(QA->GetSlotItem(i));}
 return UGameplayStatics::SaveGameToSlot(Save,SlotName,UserIndex);
}
bool UInventorySaveSubsystem::LoadInventory(UInventoryComponent* Inventory,const FString& SlotName)
{
 if(!Inventory||SlotName.IsEmpty()||!UGameplayStatics::DoesSaveGameExist(SlotName,UserIndex))return false;
 UInventorySaveGame* Save=Cast<UInventorySaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName,UserIndex));if(!Save)return false;
 if(!Inventory->RestoreFromSavedItems(Save->Items))return false;
 if(AActor* Owner=Inventory->GetOwner())if(UQuickAccessComponent* QA=Owner->FindComponentByClass<UQuickAccessComponent>())QA->RestoreQuickSlots(Save->QuickSlots,Save->ActiveQuickSlot);
 return true;
}
