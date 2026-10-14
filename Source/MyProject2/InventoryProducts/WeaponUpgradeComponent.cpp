#include "WeaponUpgradeComponent.h"
UWeaponUpgradeComponent::UWeaponUpgradeComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UWeaponUpgradeComponent::ApplyUpgrade(EWeaponUpgradeType T,FName ID){if(T==EWeaponUpgradeType::None||ID.IsNone())return false;ActiveUpgrades.Add(T,ID);return true;}
bool UWeaponUpgradeComponent::RemoveUpgrade(EWeaponUpgradeType T){return ActiveUpgrades.Remove(T)>0;}
float UWeaponUpgradeComponent::GetDamageMultiplier()const{float M=1;if(ActiveUpgrades.Contains(EWeaponUpgradeType::Fire))M+=0.3f;if(ActiveUpgrades.Contains(EWeaponUpgradeType::Poison))M+=0.2f;if(ActiveUpgrades.Contains(EWeaponUpgradeType::Bleeding))M+=0.25f;if(ActiveUpgrades.Contains(EWeaponUpgradeType::Shock))M+=0.4f;if(ActiveUpgrades.Contains(EWeaponUpgradeType::Sharpened))M+=0.5f;return M;}
