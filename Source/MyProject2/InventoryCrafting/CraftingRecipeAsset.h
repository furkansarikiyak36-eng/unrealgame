#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CraftingTypes.h"
#include "CraftingRecipeAsset.generated.h"

class UTexture2D;
UCLASS(BlueprintType)
class MYPROJECT2_API UCraftingRecipeAsset:public UPrimaryDataAsset
{GENERATED_BODY() public: UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) FName RecipeID; UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) FText DisplayName; UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) FText Description; UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) EInventoryCategory Category=EInventoryCategory::Tool; UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TArray<FInventoryCraftingIngredient> Ingredients; UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) FCraftingOutput Output; UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TArray<FCraftingCondition> Conditions; UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) bool bDiscoveredByDefault=false; UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) bool bAutoDiscoverOnAttempt=true; UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TSoftObjectPtr<UTexture2D> RecipeIcon; UFUNCTION(BlueprintCallable)bool ValidateRecipe(FString&OutError)const;};
UCLASS() class MYPROJECT2_API UCraftingRecipeLibrary:public UObject
{GENERATED_BODY() public:UFUNCTION(BlueprintCallable)static UCraftingRecipeAsset*FindRecipe(FName RecipeID);UFUNCTION(BlueprintCallable)static void RegisterRecipe(UCraftingRecipeAsset*Recipe);UFUNCTION(BlueprintCallable)static TArray<UCraftingRecipeAsset*>GetAllRecipes();private:static TMap<FName,TWeakObjectPtr<UCraftingRecipeAsset>> RecipeRegistry;};
