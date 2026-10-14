#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingTypes.h"
#include "CombiningAreaComponent.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombiningAreaChanged);
UCLASS(ClassGroup=(Crafting),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UCombiningAreaComponent:public UActorComponent
{GENERATED_BODY() public:UCombiningAreaComponent();UFUNCTION(BlueprintCallable)bool AddIngredient(FName ItemID,int32 Count=1);UFUNCTION(BlueprintCallable)bool RemoveIngredient(FName ItemID,int32 Count=1);UFUNCTION(BlueprintCallable)void ClearAll();UFUNCTION(BlueprintPure)TArray<FInventoryCraftingIngredient>GetIngredients()const{return Ingredients;}UFUNCTION(BlueprintPure)int32 GetIngredientCount(FName ItemID)const;UFUNCTION(BlueprintPure)bool IsEmpty()const{return Ingredients.IsEmpty();}UPROPERTY(EditAnywhere,BlueprintReadWrite)int32 MaxSlots=6;UPROPERTY(BlueprintAssignable)FOnCombiningAreaChanged OnCombiningAreaChanged;protected:UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly)TArray<FInventoryCraftingIngredient>Ingredients;};
