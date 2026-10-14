// MaterialDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MaterialCore/MaterialTypes.h"
#include "MaterialProperties.h"
class UMaterialInterface;
class USoundBase;

#include "MaterialDefinition.generated.h"

USTRUCT(BlueprintType)
struct FMaterialReactionProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) EMaterialReactionType Reaction = EMaterialReactionType::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Threshold = 10.0f;       // Tetiklenme eşiği
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float DamageMultiplier = 1.0f; // Hasar çarpanı
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float TimeToReact = 0.0f;      // Tepki süresi (saat)
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsReversible = false;    // Geri dönüşümlü mü?
};

UCLASS(BlueprintType)
class MYPROJECT2_API UMaterialDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FName MaterialID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    EMaterialClass MaterialClass = EMaterialClass::Stone;

    // Ana özellik seti (12 özellik)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
    FMaterialProperties Properties;

    // Darbe tepkileri (küçük, orta, büyük darbe)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactions")
    TMap<FName, FMaterialReactionProfile> ImpactReactions;

    // Ateş tepkileri
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactions")
    FMaterialReactionProfile FireReaction;

    // Su tepkileri
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactions")
    FMaterialReactionProfile WaterReaction;

    // Zaman/çürüme tepkileri
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactions")
    FMaterialReactionProfile TimeReaction;

    // Görsel
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TSoftObjectPtr<UMaterialInterface> BaseMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TSoftObjectPtr<UMaterialInterface> DamagedMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TSoftObjectPtr<UMaterialInterface> DestroyedMaterial;

    // Ses
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<USoundBase> ImpactSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<USoundBase> CrackSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<USoundBase> CollapseSound;

    // Doğrulama
    UFUNCTION(BlueprintCallable, Category = "Validation")
    bool ValidateDefinition(FString& OutError) const;

    // Yardımcılar
    UFUNCTION(BlueprintPure, Category = "Query")
    bool CanBurn() const { return Properties.Flammability > 0.1f; }

    UFUNCTION(BlueprintPure, Category = "Query")
    bool CanRot() const { return Properties.DecayRate > 0.001f; }

    UFUNCTION(BlueprintPure, Category = "Query")
    bool IsWaterSensitive() const { return Properties.WaterDamageRate > 0.1f; }

    UFUNCTION(BlueprintCallable, Category = "Query")
    FMaterialReactionProfile GetReactionFor(EMaterialPropertyKind Property) const;

    UFUNCTION(BlueprintCallable, Category = "Query")
    FMaterialReactionProfile GetImpactReactionForDamage(float DamageAmount) const;
};
