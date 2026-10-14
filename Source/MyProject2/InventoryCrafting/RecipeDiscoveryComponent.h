#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecipeDiscoveryComponent.generated.h"
class UCraftingRecipeAsset;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecipeDiscovered,UCraftingRecipeAsset*,Recipe);
UCLASS(ClassGroup=(Crafting),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API URecipeDiscoveryComponent:public UActorComponent
{GENERATED_BODY() public:URecipeDiscoveryComponent();UFUNCTION(BlueprintCallable)bool DiscoverRecipe(FName RecipeID);UFUNCTION(BlueprintCallable)bool DiscoverByAttempt(UCraftingRecipeAsset*Recipe);UFUNCTION(BlueprintPure)bool IsDiscovered(FName RecipeID)const;UFUNCTION(BlueprintPure)TArray<UCraftingRecipeAsset*>GetDiscoveredRecipes()const;UFUNCTION(BlueprintPure)TArray<UCraftingRecipeAsset*>GetUndiscoveredRecipes()const;UPROPERTY(BlueprintAssignable)FOnRecipeDiscovered OnRecipeDiscovered;UPROPERTY(EditAnywhere,BlueprintReadWrite)TArray<TObjectPtr<UCraftingRecipeAsset>>KnownRecipes;protected:UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly)TSet<FName>DiscoveredRecipeIDs;};
