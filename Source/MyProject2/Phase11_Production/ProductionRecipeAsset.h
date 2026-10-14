#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProductionChainTypes.h"
#include "ProductionRecipeAsset.generated.h"

UCLASS(BlueprintType)
class MYPROJECT2_API UProductionRecipeAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production|Recipe")
    FName RecipeID = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production|Recipe")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production|Recipe")
    TArray<FProductionIngredient> Inputs;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production|Recipe")
    TArray<FProductionOutput> Outputs;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production|Recipe", meta=(ClampMin="0.01", ClampMax="86400.0"))
    float BaseDurationSeconds = 5.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production|Recipe", meta=(ClampMin="0.0", ClampMax="100000000.0"))
    float RequiredPowerWatts = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production|Recipe", meta=(ClampMin="0", ClampMax="1000"))
    int32 RequiredWorkers = 0;

    UFUNCTION(BlueprintCallable, Category="Production|Recipe")
    bool ValidateRecipe(FString& OutError) const;
};
