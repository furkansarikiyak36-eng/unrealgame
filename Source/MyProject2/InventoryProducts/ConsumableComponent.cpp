#include "ConsumableComponent.h"
#include "InventoryCore/InventoryComponent.h"
UConsumableComponent::UConsumableComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UConsumableComponent::Consume(FName ID){UInventoryComponent*I=GetOwner()?GetOwner()->FindComponentByClass<UInventoryComponent>():nullptr;const FConsumableData*D=ConsumableDatabase.Find(ID);if(!I||!D||!I->HasItem(ID,1))return false;for(const auto&E:D->Effects){/* Bind effects to the project's health, stamina, hunger, thirst and status components. */}return I->RemoveItem(ID,1)==EInventoryResult::Success;}
TArray<FConsumableEffectData>UConsumableComponent::GetEffectsFor(FName ID)const{if(const auto*D=ConsumableDatabase.Find(ID))return D->Effects;return {};}
