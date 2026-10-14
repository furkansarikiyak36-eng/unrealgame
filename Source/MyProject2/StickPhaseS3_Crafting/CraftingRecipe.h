#pragma once
#include "CoreMinimal.h"
#include "StickTypes.h"
#include "CraftingRecipe.generated.h"
USTRUCT(BlueprintType) struct FCraftingIngredient{GENERATED_BODY() UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ItemName; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Count=1; UPROPERTY(EditAnywhere,BlueprintReadWrite) FName RequiredShape;};
USTRUCT(BlueprintType) struct FCraftingRecipe{GENERATED_BODY() UPROPERTY(EditAnywhere,BlueprintReadWrite) FName RecipeName; UPROPERTY(EditAnywhere,BlueprintReadWrite) FName OutputItem; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 OutputCount=1; UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FCraftingIngredient> Ingredients; UPROPERTY(EditAnywhere,BlueprintReadWrite) float CraftTime=2.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bRequiresTool=false; UPROPERTY(EditAnywhere,BlueprintReadWrite) FName RequiredTool;};
