#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
class UStaticMeshComponent;
#include "DryingRack.generated.h"
USTRUCT(BlueprintType) struct FDryingItem{GENERATED_BODY() UPROPERTY(BlueprintReadWrite) FName ItemName; UPROPERTY(BlueprintReadWrite) float Progress=0; UPROPERTY(BlueprintReadWrite) bool bReady=false;};
UCLASS() class MYPROJECT2_API ADryingRack:public AActor{GENERATED_BODY() public: ADryingRack(); UFUNCTION(BlueprintCallable) bool HangItem(FName ItemName); UFUNCTION(BlueprintCallable) FName CollectItem(int32 SlotIndex); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RackMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxSlots=6; UPROPERTY(EditAnywhere,BlueprintReadWrite) float DryingTime=120; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TArray<FDryingItem> HangingItems; protected: virtual void Tick(float DeltaTime)override;};
