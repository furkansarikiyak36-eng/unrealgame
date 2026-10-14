#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArmorComponent.generated.h"
UENUM(BlueprintType)enum class EArmorSlot:uint8{Head,Chest,Legs,Hands};
USTRUCT(BlueprintType)struct FArmorPiece{GENERATED_BODY() UPROPERTY(EditAnywhere,BlueprintReadWrite)FName ItemID=NAME_None;UPROPERTY(EditAnywhere,BlueprintReadWrite)EArmorSlot Slot=EArmorSlot::Chest;UPROPERTY(EditAnywhere,BlueprintReadWrite)float Defense=10;UPROPERTY(EditAnywhere,BlueprintReadWrite)float Durability=100;UPROPERTY(EditAnywhere,BlueprintReadWrite)float CurrentDurability=100;};
UCLASS(ClassGroup=(Inventory),meta=(BlueprintSpawnableComponent))class MYPROJECT2_API UArmorComponent:public UActorComponent
{GENERATED_BODY()public:UArmorComponent();UFUNCTION(BlueprintCallable)bool EquipArmor(const FArmorPiece&Piece);UFUNCTION(BlueprintCallable)bool UnequipArmor(EArmorSlot Slot);UFUNCTION(BlueprintCallable)float ApplyDamage(float Damage);UFUNCTION(BlueprintPure)float GetTotalDefense()const;UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly)TMap<EArmorSlot,FArmorPiece>EquippedArmor;};
