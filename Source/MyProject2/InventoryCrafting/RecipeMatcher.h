#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingTypes.h"
#include "RecipeMatcher.generated.h"
class UCraftingRecipeAsset;
USTRUCT(BlueprintType) struct FRecipeMatch{GENERATED_BODY() UPROPERTY(BlueprintReadWrite) TObjectPtr<UCraftingRecipeAsset>Recipe=nullptr; UPROPERTY(BlueprintReadWrite) bool bExactMatch=false; UPROPERTY(BlueprintReadWrite) int32 MissingCount=0; UPROPERTY(BlueprintReadWrite) TArray<FName>MissingItems;};
UCLASS(ClassGroup=(Crafting),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API URecipeMatcher:public UActorComponent
{GENERATED_BODY() public:URecipeMatcher();virtual void BeginPlay()override;UFUNCTION(BlueprintCallable)TArray<FRecipeMatch>FindMatchingRecipes(const TArray<FInventoryCraftingIngredient>& Ingredients)const;UFUNCTION(BlueprintCallable)FRecipeMatch FindBestMatch(const TArray<FInventoryCraftingIngredient>& Ingredients)const;UFUNCTION(BlueprintCallable)TArray<UCraftingRecipeAsset*>GetPossibleResults(const TArray<FInventoryCraftingIngredient>& Ingredients)const;UPROPERTY(EditAnywhere,BlueprintReadOnly)TArray<TObjectPtr<UCraftingRecipeAsset>>RecipeAssets;UPROPERTY(EditAnywhere,BlueprintReadWrite)bool bAllowPartialMatch=true;UPROPERTY(EditAnywhere,BlueprintReadWrite)bool bRequireToolPresence=true;protected:bool IsRecipeSatisfied(const UCraftingRecipeAsset*Recipe,const TArray<FInventoryCraftingIngredient>&Ingredients,TArray<FName>&OutMissing)const;};
