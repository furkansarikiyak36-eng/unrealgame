#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CookingComponent.generated.h"

USTRUCT(BlueprintType)
struct FCookingRecipe
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName InputItem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName OutputItem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float CookTime = 30.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RequiredHeat = 30.0f;
};

UCLASS(ClassGroup=(Cooking), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UCookingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCookingComponent();
    UFUNCTION(BlueprintCallable) bool StartCooking(FName RecipeName);
    UFUNCTION(BlueprintCallable) FName FinishCooking();
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCookingRecipe> Recipes;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) FName CurrentRecipe;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float CookProgress = 0.0f;
protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    FName CurrentInputItem = NAME_None;
    float ActiveRequiredHeat = 30.0f;
};
