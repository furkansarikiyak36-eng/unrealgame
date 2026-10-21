#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickTypes.h"
#include "StickInventoryComponent.generated.h"
UCLASS(ClassGroup=(Stick),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStickInventoryComponent:public UActorComponent
{
    GENERATED_BODY()
public:
    UStickInventoryComponent();
    UFUNCTION(BlueprintCallable) int32 AddSticks(FName Type, int32 Count);
    UFUNCTION(BlueprintCallable) int32 RemoveSticks(FName Type, int32 Count);
    UFUNCTION(BlueprintPure) int32 GetCount(FName Type) const;
    UFUNCTION(BlueprintPure) TArray<FStickBundle> GetAllBundles() const { return Bundles; }

    // Native transaction API used by crafting. Preflight aggregates duplicate inputs and
    // evaluates output capacity after the consumed inputs are removed.
    bool CanApplyCraftTransaction(const TArray<FStickBundle>& RequiredInputs, FName OutputType, int32 OutputCount) const;
    bool ApplyCraftTransaction(const TArray<FStickBundle>& RequiredInputs, FName OutputType, int32 OutputCount);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 GlobalCapacity=100;
protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TArray<FStickBundle> Bundles;
};
