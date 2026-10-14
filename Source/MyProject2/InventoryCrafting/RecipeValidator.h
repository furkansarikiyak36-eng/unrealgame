#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingTypes.h"
#include "RecipeValidator.generated.h"
class UCraftingRecipeAsset; class UInventoryComponent; class UCombiningAreaComponent;
UCLASS(ClassGroup=(Crafting),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API URecipeValidator:public UActorComponent
{GENERATED_BODY() public:URecipeValidator();UFUNCTION(BlueprintCallable)FCraftingResult ValidateCraft(UCraftingRecipeAsset*Recipe,UInventoryComponent*Inventory,UCombiningAreaComponent*Area)const;UFUNCTION(BlueprintCallable)bool CanCraft(UCraftingRecipeAsset*Recipe,UInventoryComponent*Inventory)const;UFUNCTION(BlueprintCallable)TArray<FName>GetMissingItems(UCraftingRecipeAsset*Recipe,UInventoryComponent*Inventory)const;protected:bool CheckConditions(const UCraftingRecipeAsset*Recipe)const;bool HasRequiredTools(const UCraftingRecipeAsset*Recipe,UInventoryComponent*Inventory)const;bool AreaMatches(const UCraftingRecipeAsset*Recipe,const UCombiningAreaComponent*Area)const;};
