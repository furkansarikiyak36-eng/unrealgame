#include "ProductionRecipeAsset.h"

bool UProductionRecipeAsset::ValidateRecipe(FString& OutError) const
{
    if (RecipeID.IsNone())
    {
        OutError = TEXT("RecipeID is empty.");
        return false;
    }
    if (Inputs.IsEmpty() || Outputs.IsEmpty())
    {
        OutError = TEXT("Production requires at least one input and one output.");
        return false;
    }
    if (!FMath::IsFinite(BaseDurationSeconds) || BaseDurationSeconds < 0.01f || BaseDurationSeconds > 86400.0f)
    {
        OutError = TEXT("BaseDurationSeconds must be finite and between 0.01 and 86400.");
        return false;
    }
    if (!FMath::IsFinite(RequiredPowerWatts) || RequiredPowerWatts < 0.0f || RequiredPowerWatts > 100000000.0f)
    {
        OutError = TEXT("RequiredPowerWatts is outside the supported range.");
        return false;
    }
    if (RequiredWorkers < 0 || RequiredWorkers > 1000)
    {
        OutError = TEXT("RequiredWorkers is outside the supported range.");
        return false;
    }

    for (const FProductionIngredient& Input : Inputs)
    {
        if (Input.ItemID.IsNone() || Input.Count <= 0)
        {
            OutError = TEXT("Production input contains an empty item ID or non-positive amount.");
            return false;
        }
    }
    for (const FProductionOutput& Output : Outputs)
    {
        if (Output.ItemID.IsNone() || Output.Count <= 0)
        {
            OutError = TEXT("Production output contains an empty item ID or non-positive amount.");
            return false;
        }
    }

    OutError.Empty();
    return true;
}
