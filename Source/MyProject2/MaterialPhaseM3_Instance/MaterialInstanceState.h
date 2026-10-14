// MaterialInstanceState.h
#pragma once

#include "CoreMinimal.h"
#include "MaterialCore/MaterialTypes.h"
#include "MaterialInstanceState.generated.h"

USTRUCT(BlueprintType)
struct FMaterialInstanceState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) FName MaterialID;

    // Mevcut değerler (0-1 normalize)
    UPROPERTY(BlueprintReadWrite) float CurrentMoisture = 0.3f;
    UPROPERTY(BlueprintReadWrite) float CurrentTemperature = 20.0f;
    UPROPERTY(BlueprintReadWrite) float CurrentDamage = 0.0f;
    UPROPERTY(BlueprintReadWrite) float StructuralHealth = 1.0f;

    // Yangın durumu
    UPROPERTY(BlueprintReadWrite) float BurnProgress = 0.0f;
    UPROPERTY(BlueprintReadWrite) bool bIsBurning = false;

    // Çürüme durumu
    UPROPERTY(BlueprintReadWrite) float DecayProgress = 0.0f;

    // Aşınma durumu
    UPROPERTY(BlueprintReadWrite) float WeatheringLevel = 0.0f;

    // Son güncelleme
    UPROPERTY(BlueprintReadWrite) float LastUpdateDay = 1.0f;

    // Aktif reaksiyonlar
    UPROPERTY(BlueprintReadWrite) TArray<EMaterialReactionType> ActiveReactions;

    bool IsDamaged() const { return CurrentDamage > 0.1f; }

    bool IsBurning() const { return bIsBurning; }

    bool IsDecaying() const { return DecayProgress > 0.01f; }

    float GetHealthPercent() const { return FMath::Clamp(1.0f - CurrentDamage, 0.0f, 1.0f); }

    void ApplyDamage(float Amount)
    {
        CurrentDamage = FMath::Clamp(CurrentDamage + Amount, 0.0f, 1.0f);
        StructuralHealth = FMath::Clamp(1.0f - CurrentDamage, 0.0f, 1.0f);
    }

    void Reset()
    {
        CurrentMoisture = 0.3f;
        CurrentTemperature = 20.0f;
        CurrentDamage = 0.0f;
        StructuralHealth = 1.0f;
        BurnProgress = 0.0f;
        bIsBurning = false;
        DecayProgress = 0.0f;
        WeatheringLevel = 0.0f;
        ActiveReactions.Empty();
    }
};
