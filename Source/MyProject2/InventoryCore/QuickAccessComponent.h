#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryTypes.h"
#include "QuickAccessComponent.generated.h"
UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UQuickAccessComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UQuickAccessComponent(); virtual void BeginPlay() override;
 UFUNCTION(BlueprintCallable) bool AssignToSlot(FName ItemID,int32 SlotIndex);
 UFUNCTION(BlueprintCallable) bool ClearSlot(int32 SlotIndex);
 UFUNCTION(BlueprintPure) FName GetSlotItem(int32 SlotIndex) const;
 UFUNCTION(BlueprintPure) int32 GetActiveSlot() const{return ActiveSlot;}
 UFUNCTION(BlueprintCallable) bool SelectSlot(int32 SlotIndex);
 UFUNCTION(BlueprintCallable) bool CycleNext(); UFUNCTION(BlueprintCallable) bool CyclePrevious();
 UFUNCTION(BlueprintCallable) bool RestoreQuickSlots(const TArray<FName>& Items,int32 InActiveSlot);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 SlotCount=8;
 UPROPERTY(BlueprintAssignable) FOnItemEquipped OnItemEquipped;
protected:
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TArray<FName> QuickSlots;
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) int32 ActiveSlot=INDEX_NONE;
};
