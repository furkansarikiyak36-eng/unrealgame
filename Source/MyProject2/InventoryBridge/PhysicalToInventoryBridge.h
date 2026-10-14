#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicalToInventoryBridge.generated.h"
class ALogActor;class AStoneActor;class AStickActor;class UInventoryComponent;
UCLASS(ClassGroup=(Bridge),meta=(BlueprintSpawnableComponent))class MYPROJECT2_API UPhysicalToInventoryBridge:public UActorComponent
{GENERATED_BODY()public:UPhysicalToInventoryBridge();UFUNCTION(BlueprintCallable)bool ConvertLogToItem(ALogActor*Log,FName ItemID);UFUNCTION(BlueprintCallable)bool ConvertStoneToItem(AStoneActor*Stone,FName ItemID);UFUNCTION(BlueprintCallable)bool ConvertStickToItem(AStickActor*Stick,FName ItemID);UPROPERTY(EditAnywhere,BlueprintReadWrite)bool bDestroyPhysicalAfterConvert=true;protected:UInventoryComponent*GetInventory()const;};
