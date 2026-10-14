// MaterialProperties.h
#pragma once

#include "CoreMinimal.h"
#include "MaterialCore/MaterialTypes.h"
#include "MaterialProperties.generated.h"

USTRUCT(BlueprintType)
struct FMaterialProperties
{
    GENERATED_BODY()

    // 1. Yoğunluk (kg/m³) — gerçek birim, 0-100 arası normalize da olabilir
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float Density = 500.0f;

    // 2. Sertlik (Mohs benzeri 0-10 veya normalize 0-100)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float Hardness = 50.0f;

    // 3. Darbe dayanımı (0-100)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float ImpactResistance = 50.0f;

    // 4. Basınç dayanımı (0-100)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float CompressionStrength = 50.0f;

    // 5. Çekme dayanımı (0-100)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float TensileStrength = 30.0f;

    // 6. Gevreklik (0-1) — yüksek = ani kırılma
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float Brittleness = 0.5f;

    // 7. Yanıcılık (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float Flammability = 0.0f;

    // 8. Yakıt değeri (MJ/kg benzeri)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float FuelValue = 0.0f;

    // 9. Su emme (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float WaterAbsorption = 0.1f;

    // 10. Su hasarı (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float WaterDamageRate = 0.0f;

    // 11. Çürüme hızı (0-1 / gün)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float DecayRate = 0.0f;

    // 12. İşlenebilirlik (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
    float Workability = 0.5f;

    float GetProperty(EMaterialPropertyKind Property) const;

    void SetProperty(EMaterialPropertyKind Property, float Value);

    bool IsFlammable() const { return Flammability > 0.1f; }

    bool IsWaterSensitive() const { return WaterDamageRate > 0.1f; }

    bool IsDecaying() const { return DecayRate > 0.001f; }

    // Malzeme karşılaştırması: bu malzeme diğerinden ne kadar sert?
    float CompareHardness(const FMaterialProperties& Other) const;
};
