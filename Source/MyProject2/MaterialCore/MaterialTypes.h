// MaterialTypes.h
#pragma once

#include "CoreMinimal.h"
#include "MaterialTypes.generated.h"

UENUM(BlueprintType)
enum class EMaterialClass : uint8
{
    Stone       UMETA(DisplayName = "Taş"),
    Wood        UMETA(DisplayName = "Odun"),
    Plaster     UMETA(DisplayName = "Alçı"),
    Soil        UMETA(DisplayName = "Toprak"),
    Metal       UMETA(DisplayName = "Metal"),
    Rope        UMETA(DisplayName = "Halat"),
    Mortar      UMETA(DisplayName = "Harç"),
    Glass       UMETA(DisplayName = "Cam"),
    Ceramic     UMETA(DisplayName = "Seramik")
};

UENUM(BlueprintType)
enum class EMaterialPropertyKind : uint8
{
    Density             UMETA(DisplayName = "Yoğunluk"),
    Hardness            UMETA(DisplayName = "Sertlik"),
    ImpactResistance    UMETA(DisplayName = "Darbe Dayanımı"),
    CompressionStrength UMETA(DisplayName = "Basınç Dayanımı"),
    TensileStrength     UMETA(DisplayName = "Çekme Dayanımı"),
    Brittleness         UMETA(DisplayName = "Gevreklik"),
    Flammability        UMETA(DisplayName = "Yanıcılık"),
    FuelValue           UMETA(DisplayName = "Yakıt Değeri"),
    WaterAbsorption     UMETA(DisplayName = "Su Emme"),
    WaterDamageRate     UMETA(DisplayName = "Su Hasarı"),
    DecayRate           UMETA(DisplayName = "Çürüme Hızı"),
    Workability         UMETA(DisplayName = "İşlenebilirlik")
};

UENUM(BlueprintType)
enum class EMaterialReactionType : uint8
{
    None        UMETA(DisplayName = "Yok"),
    Crack       UMETA(DisplayName = "Çatlak"),
    Shatter     UMETA(DisplayName = "Parçalanma"),
    Deform      UMETA(DisplayName = "Deformasyon"),
    Burn        UMETA(DisplayName = "Yanma"),
    Char        UMETA(DisplayName = "Kömürleşme"),
    Dissolve    UMETA(DisplayName = "Çözünme"),
    Swell       UMETA(DisplayName = "Şişme"),
    Rot         UMETA(DisplayName = "Çürüme"),
    Rust        UMETA(DisplayName = "Paslanma"),
    Erode       UMETA(DisplayName = "Aşınma"),
    Crush       UMETA(DisplayName = "Ezilme"),
    Melt        UMETA(DisplayName = "Erime")
};

UENUM(BlueprintType)
enum class EStructureLayerRole : uint8
{
    None        UMETA(DisplayName = "Yok"),
    Foundation  UMETA(DisplayName = "Temel"),
    Skeleton    UMETA(DisplayName = "İskelet"),
    Cladding    UMETA(DisplayName = "Kaplama"),
    Roof        UMETA(DisplayName = "Çatı"),
    Connector   UMETA(DisplayName = "Bağlantı"),
    Insulation  UMETA(DisplayName = "Yalıtım"),
    Facing      UMETA(DisplayName = "Yüzey")
};

UENUM(BlueprintType)
enum class EStructureLayerCondition : uint8
{
    Pristine    UMETA(DisplayName = "Sağlam"),
    Damaged     UMETA(DisplayName = "Hasarlı"),
    Weakened    UMETA(DisplayName = "Zayıf"),
    Failing     UMETA(DisplayName = "Çökmek Üzere"),
    Collapsed   UMETA(DisplayName = "Çökmüş")
};

UENUM(BlueprintType)
enum class EWeatheringType : uint8
{
    None        UMETA(DisplayName = "Yok"),
    Rain        UMETA(DisplayName = "Yağmur"),
    Snow        UMETA(DisplayName = "Kar"),
    Wind        UMETA(DisplayName = "Rüzgar"),
    Sun         UMETA(DisplayName = "Güneş"),
    Frost       UMETA(DisplayName = "Don")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMaterialDamageApplied, FName, MaterialID, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMaterialReaction, FName, MaterialID, EMaterialReactionType, Reaction, float, Intensity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLayerConditionChanged, EStructureLayerRole, Role, EStructureLayerCondition, NewCondition);
