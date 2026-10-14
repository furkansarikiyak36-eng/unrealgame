#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemDropComponent.generated.h"
class UInventoryComponent;
UCLASS(ClassGroup=(Inventory),meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UItemDropComponent:public UActorComponent
{GENERATED_BODY() public:UItemDropComponent();UFUNCTION(BlueprintCallable)bool DropItem(FName ItemID,int32 Count=1);UFUNCTION(BlueprintCallable)bool DropAllOfItem(FName ItemID);UPROPERTY(EditAnywhere,BlueprintReadWrite)float DropForwardDistance=150.0f;UPROPERTY(EditAnywhere,BlueprintReadWrite)float DropImpulse=200.0f;protected:UInventoryComponent*GetInventory()const;};
