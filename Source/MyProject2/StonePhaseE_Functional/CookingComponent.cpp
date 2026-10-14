#include "CookingComponent.h"
#include "HeatRadiusComponent.h"
UCookingComponent::UCookingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.5f;
}
bool UCookingComponent::StartCooking(FName RecipeName)
{
    for (const FCookingRecipe& R : Recipes)
    {
        if (R.InputItem == RecipeName)
        {
            CurrentRecipe = RecipeName;
            CurrentInputItem = R.InputItem;
            ActiveRequiredHeat = R.RequiredHeat;
            CookProgress = 0.0f;
            return true;
        }
    }
    return false;
}
FName UCookingComponent::FinishCooking()
{
    for (const FCookingRecipe& R : Recipes)
    {
        if (R.InputItem == CurrentInputItem && !CurrentInputItem.IsNone())
        {
            const FName Output = R.OutputItem;
            CurrentRecipe = NAME_None; CurrentInputItem = NAME_None; CookProgress = 0.0f;
            return Output;
        }
    }
    return NAME_None;
}
void UCookingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (CurrentRecipe.IsNone() || !GetOwner()) return;
    UHeatRadiusComponent* Heat = GetOwner()->FindComponentByClass<UHeatRadiusComponent>();
    if (Heat && Heat->GetHeatAtLocation(GetOwner()->GetActorLocation()) < ActiveRequiredHeat) return;
    CookProgress += DeltaTime;
    for (const FCookingRecipe& R : Recipes)
        if (R.InputItem == CurrentInputItem && CookProgress >= R.CookTime) { FinishCooking(); return; }
}
