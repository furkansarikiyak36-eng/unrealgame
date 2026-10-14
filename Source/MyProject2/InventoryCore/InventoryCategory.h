#pragma once
#include "CoreMinimal.h"
#include "InventoryCategory.generated.h"

UENUM(BlueprintType)
enum class EInventoryCategory : uint8
{
    Resource UMETA(DisplayName="Kaynak"), Consumable UMETA(DisplayName="Tüketilebilir"),
    Tool UMETA(DisplayName="Alet"), Weapon UMETA(DisplayName="Silah"),
    Armor UMETA(DisplayName="Zırh"), Ammo UMETA(DisplayName="Mühimmat"),
    Quest UMETA(DisplayName="Görev"), Miscellaneous UMETA(DisplayName="Çeşitli")
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
    Common UMETA(DisplayName="Yaygın"), Uncommon UMETA(DisplayName="Az Bulunur"),
    Rare UMETA(DisplayName="Nadir"), Legendary UMETA(DisplayName="Efsanevi")
};

UENUM(BlueprintType)
enum class EInventoryResult : uint8
{
    Success UMETA(DisplayName="Başarılı"), NotEnoughItems UMETA(DisplayName="Yetersiz Malzeme"),
    NoRecipeMatch UMETA(DisplayName="Tarif Eşleşmedi"), InventoryFull UMETA(DisplayName="Envanter Dolu"),
    MissingTool UMETA(DisplayName="Eksik Alet"), Cancelled UMETA(DisplayName="İptal Edildi"),
    InvalidRequest UMETA(DisplayName="Geçersiz İstek"), TooHeavy UMETA(DisplayName="Taşıma Ağırlığı Aşıldı")
};
