#include "CraftingComponent.h"
#include "StickPhaseS1_Core/StickInventoryComponent.h"

namespace
{
    const FCraftingRecipe* FindRecipeByName(const TArray<FCraftingRecipe>& Recipes, FName Name)
    {
        for (const FCraftingRecipe& Recipe : Recipes)
        {
            if (Recipe.RecipeName == Name)
            {
                return &Recipe;
            }
        }
        return nullptr;
    }

    bool BuildRequiredInputs(const FCraftingRecipe& Recipe, TArray<FStickBundle>& OutInputs)
    {
        OutInputs.Reset();
        for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
        {
            // The stick inventory stores counts and item names, not per-piece shape state.
            // Refuse unsupported shape-specific recipes instead of silently ignoring the constraint.
            if (Ingredient.ItemName.IsNone() || Ingredient.Count <= 0 || !Ingredient.RequiredShape.IsNone())
            {
                return false;
            }

            FStickBundle Input;
            Input.StickType = Ingredient.ItemName;
            Input.Count = Ingredient.Count;
            OutInputs.Add(Input);
        }
        return !Recipe.OutputItem.IsNone() && Recipe.OutputCount > 0;
    }
}

UCraftingComponent::UCraftingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UCraftingComponent::CanCraft(FName RecipeName) const
{
    UStickInventoryComponent* Inventory = GetOwner()
        ? GetOwner()->FindComponentByClass<UStickInventoryComponent>()
        : nullptr;
    if (!Inventory)
    {
        return false;
    }

    const FCraftingRecipe* Recipe = FindRecipeByName(Recipes, RecipeName);
    if (!Recipe)
    {
        return false;
    }

    if (Recipe->bRequiresTool &&
        (Recipe->RequiredTool.IsNone() || Inventory->GetCount(Recipe->RequiredTool) <= 0))
    {
        return false;
    }

    TArray<FStickBundle> RequiredInputs;
    if (!BuildRequiredInputs(*Recipe, RequiredInputs))
    {
        return false;
    }

    return Inventory->CanApplyCraftTransaction(RequiredInputs, Recipe->OutputItem, Recipe->OutputCount);
}

bool UCraftingComponent::Craft(FName RecipeName)
{
    if (!CanCraft(RecipeName))
    {
        return false;
    }

    UStickInventoryComponent* Inventory = GetOwner()
        ? GetOwner()->FindComponentByClass<UStickInventoryComponent>()
        : nullptr;
    if (!Inventory)
    {
        return false;
    }

    const FCraftingRecipe* Recipe = FindRecipeByName(Recipes, RecipeName);
    if (!Recipe)
    {
        return false;
    }

    TArray<FStickBundle> RequiredInputs;
    if (!BuildRequiredInputs(*Recipe, RequiredInputs) ||
        !Inventory->ApplyCraftTransaction(RequiredInputs, Recipe->OutputItem, Recipe->OutputCount))
    {
        return false;
    }

    LastCraftedItem = Recipe->OutputItem;
    return true;
}
