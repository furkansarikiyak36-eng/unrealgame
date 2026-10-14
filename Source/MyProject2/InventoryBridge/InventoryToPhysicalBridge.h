#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryToPhysicalBridge.generated.h"
class ALogActor;class AStoneActor;class AStickActor;class UInventoryComponent;
UCLASS(ClassGroup=(Bridge),meta=(BlueprintSpawnableComponent))class MYPROJECT2_API UInventoryToPhysicalBridge:public UActorComponent
{GENERATED_BODY()public:UInventoryToPhysicalBridge();UFUNCTION(BlueprintCallable)ALogActor*SpawnLogFromInventory(FName ItemID,const FVector&Location);UFUNCTION(BlueprintCallable)AStoneActor*SpawnStoneFromInventory(FName ItemID,const FVector&Location);UFUNCTION(BlueprintCallable)AStickActor*SpawnStickFromInventory(FName ItemID,const FVector&Location);UPROPERTY(EditAnywhere,BlueprintReadWrite)TSubclassOf<ALogActor>LogClass;UPROPERTY(EditAnywhere,BlueprintReadWrite)TSubclassOf<AStoneActor>StoneClass;UPROPERTY(EditAnywhere,BlueprintReadWrite)TSubclassOf<AStickActor>StickClass;protected:UInventoryComponent*GetInventory()const;};
