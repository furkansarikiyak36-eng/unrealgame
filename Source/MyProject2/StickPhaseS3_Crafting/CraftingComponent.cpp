#include "CraftingComponent.h"
#include "StickPhaseS1_Core/StickInventoryComponent.h"
UCraftingComponent::UCraftingComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UCraftingComponent::CanCraft(FName N)const{auto*I=GetOwner()?GetOwner()->FindComponentByClass<UStickInventoryComponent>():nullptr;if(!I)return false;for(const auto&R:Recipes)if(R.RecipeName==N){for(const auto&In:R.Ingredients)if(I->GetCount(In.ItemName)<In.Count)return false;return true;}return false;}
bool UCraftingComponent::Craft(FName N){if(!CanCraft(N))return false;auto*I=GetOwner()->FindComponentByClass<UStickInventoryComponent>();for(const auto&R:Recipes)if(R.RecipeName==N){for(const auto&In:R.Ingredients)if(I->RemoveSticks(In.ItemName,In.Count)!=In.Count)return false;const int32 Added=I->AddSticks(R.OutputItem,R.OutputCount);if(Added!=R.OutputCount)return false;LastCraftedItem=R.OutputItem;return true;}return false;}
