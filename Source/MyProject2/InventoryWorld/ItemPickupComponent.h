#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemPickupComponent.generated.h"
class AWorldItemActor; class UInventoryComponent;
UCLASS(ClassGroup=(Inventory),meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UItemPickupComponent:public UActorComponent
{ GENERATED_BODY() public: UItemPickupComponent(); UFUNCTION(BlueprintCallable) bool PickupItem(AWorldItemActor* WorldItem); UFUNCTION(BlueprintCallable) int32 PickupNearbyItems(float Radius); UFUNCTION(BlueprintCallable) AWorldItemActor* FindNearestPickupable(float SearchRadius)const; UPROPERTY(EditAnywhere,BlueprintReadWrite) float AutoPickupRadius=80.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bAutoPickup=false; protected: virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)override; UInventoryComponent* GetInventory()const; };
