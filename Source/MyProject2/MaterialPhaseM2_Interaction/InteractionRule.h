// InteractionRule.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MaterialCore/MaterialTypes.h"
#include "MaterialProperties.h"
#include "InteractionRule.generated.h"

class UMaterialDefinition;

USTRUCT(BlueprintType)
struct FMaterialInteractionContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) float Magnitude = 1.0f;       // Şiddet
    UPROPERTY(BlueprintReadWrite) float DurationHours = 0.0f;   // Süre
    UPROPERTY(BlueprintReadWrite) float AmbientTemperature = 20.0f;
    UPROPERTY(BlueprintReadWrite) float Moisture = 0.5f;
    UPROPERTY(BlueprintReadWrite) FVector Direction = FVector::ZeroVector;
};

UCLASS(BlueprintType, EditInlineNew, Abstract)
class MYPROJECT2_API UInteractionRule : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    EMaterialClass TargetClass = EMaterialClass::Stone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    float BaseThreshold = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    float DamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
    bool bIsEnabled = true;

    // Ana metot: verilen malzemeye göre sonuç üret
    UFUNCTION(BlueprintCallable, Category = "Rule")
    virtual float EvaluateDamage(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const;

    // Tepki tipi
    UFUNCTION(BlueprintCallable, Category = "Rule")
    virtual EMaterialReactionType DetermineReaction(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const;

    // Uygulanabilir mi?
    UFUNCTION(BlueprintCallable, Category = "Rule")
    virtual bool CanApplyTo(const UMaterialDefinition* Material) const;
};
