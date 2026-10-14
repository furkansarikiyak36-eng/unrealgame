#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryCore/InventoryTypes.h"
#include "InventoryUIComponent.generated.h"
class UInventoryComponent;
UENUM(BlueprintType)enum class EInventorySortMode:uint8{None,ByCategory,ByName,ByRarity};
UCLASS(ClassGroup=(Inventory),meta=(BlueprintSpawnableComponent))class MYPROJECT2_API UInventoryUIComponent:public UActorComponent
{GENERATED_BODY() public:UInventoryUIComponent();UFUNCTION(BlueprintCallable)void OpenInventory();UFUNCTION(BlueprintCallable)void CloseInventory();UFUNCTION(BlueprintPure)bool IsInventoryOpen()const{return bIsOpen;}UFUNCTION(BlueprintCallable)void SetCategoryFilter(EInventoryCategory Category,bool bEnable);UFUNCTION(BlueprintCallable)void SetSortMode(EInventorySortMode Mode);UFUNCTION(BlueprintCallable)TArray<FInventorySlot>GetFilteredSlots()const;UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly)bool bIsOpen=false;UPROPERTY(EditAnywhere,BlueprintReadWrite)EInventorySortMode SortMode=EInventorySortMode::ByCategory;protected:UPROPERTY()TSet<EInventoryCategory>EnabledCategories;UInventoryComponent*GetInventory()const;};
