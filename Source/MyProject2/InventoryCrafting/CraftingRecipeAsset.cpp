#include "CraftingRecipeAsset.h"
TMap<FName,TWeakObjectPtr<UCraftingRecipeAsset>> UCraftingRecipeLibrary::RecipeRegistry;
bool UCraftingRecipeAsset::ValidateRecipe(FString&OutError)const{if(RecipeID.IsNone()){OutError=TEXT("RecipeID boş");return false;}if(Ingredients.IsEmpty()){OutError=TEXT("Ingredients boş");return false;}if(Output.ItemID.IsNone()||Output.Count<=0){OutError=TEXT("Output tanımı geçersiz");return false;}for(const auto&I:Ingredients)if(I.ItemID.IsNone()||I.Count<=0){OutError=TEXT("Ingredient geçersiz");return false;}OutError.Empty();return true;}
UCraftingRecipeAsset*UCraftingRecipeLibrary::FindRecipe(FName ID){if(auto*P=RecipeRegistry.Find(ID))return P->Get();return nullptr;}
void UCraftingRecipeLibrary::RegisterRecipe(UCraftingRecipeAsset*R){if(R&&!R->RecipeID.IsNone())RecipeRegistry.Add(R->RecipeID,R);}
TArray<UCraftingRecipeAsset*>UCraftingRecipeLibrary::GetAllRecipes(){TArray<UCraftingRecipeAsset*>R;for(auto I=RecipeRegistry.CreateIterator();I;++I){if(I.Value().IsValid())R.Add(I.Value().Get());else I.RemoveCurrent();}return R;}
