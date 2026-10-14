#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventoryTypes.h"
#include "WorldItemActor.generated.h"
class UStaticMeshComponent; class USphereComponent; class UInventoryItemData;
UCLASS()
class MYPROJECT2_API AWorldItemActor : public AActor
{
 GENERATED_BODY()
public:
 AWorldItemActor();
 UFUNCTION(BlueprintCallable) void InitializeFromItemData(UInventoryItemData* InItemData,int32 Count=1);
 UFUNCTION(BlueprintPure) FInventoryItem GetItemData()const{return ItemData;}
 UFUNCTION(BlueprintPure) bool IsPickupable()const{return bPickupable;}
 UFUNCTION(BlueprintCallable) void SetPickupable(bool bNew){bPickupable=bNew;}
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> MeshComponent;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USphereComponent> InteractionSphere;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float InteractRadius=150.0f;
 UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) FInventoryItem ItemData;
protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bPickupable=true;
};
