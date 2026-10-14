#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingRecipe.h"
#include "CraftingComponent.generated.h"
UCLASS(ClassGroup=(Craft),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UCraftingComponent:public UActorComponent{GENERATED_BODY() public: UCraftingComponent(); UFUNCTION(BlueprintCallable) bool CanCraft(FName RecipeName)const; UFUNCTION(BlueprintCallable) bool Craft(FName RecipeName); UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FCraftingRecipe> Recipes; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) FName LastCraftedItem;};
