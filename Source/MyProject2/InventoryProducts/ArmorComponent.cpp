#include "ArmorComponent.h"
UArmorComponent::UArmorComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UArmorComponent::EquipArmor(const FArmorPiece&P){if(P.ItemID.IsNone()||P.Defense<0||P.Durability<=0)return false;FArmorPiece Copy=P;Copy.CurrentDurability=FMath::Clamp(Copy.CurrentDurability,0.0f,Copy.Durability);EquippedArmor.Add(P.Slot,Copy);return true;}
bool UArmorComponent::UnequipArmor(EArmorSlot S){return EquippedArmor.Remove(S)>0;}
float UArmorComponent::ApplyDamage(float D){D=FMath::Max(0.0f,D);const float Defense=GetTotalDefense();const float Absorbed=FMath::Min(D,Defense*0.5f);for(auto&Pair:EquippedArmor)Pair.Value.CurrentDurability=FMath::Max(0.0f,Pair.Value.CurrentDurability-Absorbed*0.3f);return D-Absorbed;}
float UArmorComponent::GetTotalDefense()const{float N=0;for(const auto&P:EquippedArmor)if(P.Value.CurrentDurability>0)N+=P.Value.Defense;return N;}
