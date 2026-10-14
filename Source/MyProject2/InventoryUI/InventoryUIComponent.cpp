#include "InventoryUIComponent.h"
#include "InventoryCore/InventoryComponent.h"
UInventoryUIComponent::UInventoryUIComponent(){PrimaryComponentTick.bCanEverTick=false;}
UInventoryComponent*UInventoryUIComponent::GetInventory()const{return GetOwner()?GetOwner()->FindComponentByClass<UInventoryComponent>():nullptr;}
void UInventoryUIComponent::OpenInventory(){bIsOpen=true;}void UInventoryUIComponent::CloseInventory(){bIsOpen=false;}
void UInventoryUIComponent::SetCategoryFilter(EInventoryCategory C,bool b){if(b)EnabledCategories.Add(C);else EnabledCategories.Remove(C);}
void UInventoryUIComponent::SetSortMode(EInventorySortMode M){SortMode=M;if(UInventoryComponent*I=GetInventory()){if(M==EInventorySortMode::ByCategory)I->SortByCategory();else if(M==EInventorySortMode::ByName)I->SortByName();else if(M==EInventorySortMode::ByRarity)I->SortByRarity();}}
TArray<FInventorySlot>UInventoryUIComponent::GetFilteredSlots()const{TArray<FInventorySlot>O;UInventoryComponent*I=GetInventory();if(!I)return O;for(const auto&S:I->GetSlots())if(S.IsOccupied()&&(EnabledCategories.IsEmpty()||EnabledCategories.Contains(S.Item.Category)))O.Add(S);return O;}
