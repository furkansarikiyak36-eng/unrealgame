#include "RecipeDiscoveryComponent.h"
#include "CraftingRecipeAsset.h"
URecipeDiscoveryComponent::URecipeDiscoveryComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool URecipeDiscoveryComponent::DiscoverRecipe(FName ID){if(ID.IsNone()||DiscoveredRecipeIDs.Contains(ID))return false;UCraftingRecipeAsset*R=UCraftingRecipeLibrary::FindRecipe(ID);if(!R)for(UCraftingRecipeAsset*X:KnownRecipes)if(X&&X->RecipeID==ID){R=X;UCraftingRecipeLibrary::RegisterRecipe(X);break;}if(!R)return false;DiscoveredRecipeIDs.Add(ID);if(OnRecipeDiscovered.IsBound())OnRecipeDiscovered.Broadcast(R);return true;}
bool URecipeDiscoveryComponent::DiscoverByAttempt(UCraftingRecipeAsset*R){return R&&R->bAutoDiscoverOnAttempt&&DiscoverRecipe(R->RecipeID);}
bool URecipeDiscoveryComponent::IsDiscovered(FName ID)const{return DiscoveredRecipeIDs.Contains(ID);}
TArray<UCraftingRecipeAsset*>URecipeDiscoveryComponent::GetDiscoveredRecipes()const{TArray<UCraftingRecipeAsset*>O;for(FName ID:DiscoveredRecipeIDs){if(auto*R=UCraftingRecipeLibrary::FindRecipe(ID))O.Add(R);else for(UCraftingRecipeAsset*X:KnownRecipes)if(X&&X->RecipeID==ID){O.Add(X);break;}}return O;}
TArray<UCraftingRecipeAsset*>URecipeDiscoveryComponent::GetUndiscoveredRecipes()const{TArray<UCraftingRecipeAsset*>O;for(UCraftingRecipeAsset*R:KnownRecipes)if(R&&!DiscoveredRecipeIDs.Contains(R->RecipeID))O.Add(R);for(UCraftingRecipeAsset*R:UCraftingRecipeLibrary::GetAllRecipes())if(R&&!DiscoveredRecipeIDs.Contains(R->RecipeID)&&!O.Contains(R))O.Add(R);return O;}
