#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponUpgradeComponent.generated.h"
UENUM(BlueprintType)enum class EWeaponUpgradeType:uint8{None,Poison,Fire,Bleeding,Shock,Sharpened};
UCLASS(ClassGroup=(Inventory),meta=(BlueprintSpawnableComponent))class MYPROJECT2_API UWeaponUpgradeComponent:public UActorComponent
{GENERATED_BODY()public:UWeaponUpgradeComponent();UFUNCTION(BlueprintCallable)bool ApplyUpgrade(EWeaponUpgradeType Type,FName ItemID);UFUNCTION(BlueprintCallable)bool RemoveUpgrade(EWeaponUpgradeType Type);UFUNCTION(BlueprintPure)float GetDamageMultiplier()const;UFUNCTION(BlueprintPure)float GetEffectDuration()const{return UpgradeDuration;}UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly)TMap<EWeaponUpgradeType,FName>ActiveUpgrades;UPROPERTY(EditAnywhere,BlueprintReadWrite)float UpgradeDuration=120;};
