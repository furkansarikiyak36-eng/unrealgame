#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryCrafting/CraftingTypes.h"
#include "InventoryCrafting/RecipeMatcher.h"
#include "CraftingUIComponent.generated.h"
class UCraftingRecipeAsset;class UCombiningAreaComponent;class URecipeValidator;class UInventoryComponent;class URecipeDiscoveryComponent;
UCLASS(ClassGroup=(Crafting),meta=(BlueprintSpawnableComponent))class MYPROJECT2_API UCraftingUIComponent:public UActorComponent
{GENERATED_BODY() public:UCraftingUIComponent();UFUNCTION(BlueprintCallable)void OpenCraftingUI();UFUNCTION(BlueprintCallable)void CloseCraftingUI();UFUNCTION(BlueprintCallable)TArray<FRecipeMatch>GetAvailableRecipes()const;UFUNCTION(BlueprintCallable)bool PreviewRecipe(UCraftingRecipeAsset*Recipe);UFUNCTION(BlueprintCallable)bool ConfirmCraft(UCraftingRecipeAsset*Recipe);UFUNCTION(BlueprintCallable)bool CancelCraft();UFUNCTION(BlueprintPure)bool IsOpen()const{return bIsOpen;}UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly)bool bIsOpen=false;protected:UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly)TObjectPtr<UCraftingRecipeAsset>PreviewedRecipe=nullptr;UInventoryComponent*GetInventory()const;UCombiningAreaComponent*GetArea()const;URecipeMatcher*GetMatcher()const;URecipeValidator*GetValidator()const;URecipeDiscoveryComponent*GetDiscovery()const;};
