#include "CraftingUIComponent.h"
#include "InventoryCore/InventoryComponent.h"
#include "InventoryCrafting/CombiningAreaComponent.h"
#include "InventoryCrafting/RecipeValidator.h"
#include "InventoryCrafting/RecipeDiscoveryComponent.h"
#include "InventoryCrafting/CraftingRecipeAsset.h"
UCraftingUIComponent::UCraftingUIComponent(){PrimaryComponentTick.bCanEverTick=false;}
UInventoryComponent*UCraftingUIComponent::GetInventory()const{return GetOwner()?GetOwner()->FindComponentByClass<UInventoryComponent>():nullptr;}
UCombiningAreaComponent*UCraftingUIComponent::GetArea()const{return GetOwner()?GetOwner()->FindComponentByClass<UCombiningAreaComponent>():nullptr;}
URecipeMatcher*UCraftingUIComponent::GetMatcher()const{return GetOwner()?GetOwner()->FindComponentByClass<URecipeMatcher>():nullptr;}
URecipeValidator*UCraftingUIComponent::GetValidator()const{return GetOwner()?GetOwner()->FindComponentByClass<URecipeValidator>():nullptr;}
URecipeDiscoveryComponent*UCraftingUIComponent::GetDiscovery()const{return GetOwner()?GetOwner()->FindComponentByClass<URecipeDiscoveryComponent>():nullptr;}
void UCraftingUIComponent::OpenCraftingUI(){bIsOpen=true;}void UCraftingUIComponent::CloseCraftingUI(){bIsOpen=false;PreviewedRecipe=nullptr;}
TArray<FRecipeMatch>UCraftingUIComponent::GetAvailableRecipes()const{URecipeMatcher*M=GetMatcher();UCombiningAreaComponent*A=GetArea();return M&&A?M->FindMatchingRecipes(A->GetIngredients()):TArray<FRecipeMatch>();}
bool UCraftingUIComponent::PreviewRecipe(UCraftingRecipeAsset*R){if(!R)return false;PreviewedRecipe=R;return true;}
bool UCraftingUIComponent::ConfirmCraft(UCraftingRecipeAsset*R){UInventoryComponent*I=GetInventory();URecipeValidator*V=GetValidator();UCombiningAreaComponent*A=GetArea();URecipeDiscoveryComponent*D=GetDiscovery();if(!I||!V||!R)return false;const FCraftingResult Check=V->ValidateCraft(R,I,A);if(Check.Result!=EInventoryResult::Success)return false;
// Batch duplicate ingredients and keep tool/non-consumed ingredients in inventory.
TMap<FName,int32>ConsumedMap;for(const auto&Ing:R->Ingredients)if(Ing.bConsumed&&!Ing.bIsTool)ConsumedMap.FindOrAdd(Ing.ItemID)+=Ing.Count;
TArray<TPair<FName,int32>>Consumed;for(const auto&Pair:ConsumedMap)Consumed.Emplace(Pair.Key,Pair.Value);
TArray<TPair<FName,int32>>Removed;
for(const auto&P:Consumed){if(I->RemoveItem(P.Key,P.Value)!=EInventoryResult::Success){for(const auto&Back:Removed)I->AddItem(Back.Key,Back.Value);return false;}Removed.Add(P);}
if(I->AddItem(Check.OutputItemID,Check.OutputCount)!=EInventoryResult::Success){for(const auto&P:Removed)I->AddItem(P.Key,P.Value);return false;}
if(D)D->DiscoverRecipe(R->RecipeID);if(A)A->ClearAll();PreviewedRecipe=nullptr;return true;}
bool UCraftingUIComponent::CancelCraft(){if(UCombiningAreaComponent*A=GetArea()){A->ClearAll();PreviewedRecipe=nullptr;return true;}return false;}
