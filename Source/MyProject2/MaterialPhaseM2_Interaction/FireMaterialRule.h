// FireMaterialRule.h
#pragma once
#include "MaterialCore/MaterialTypes.h"

#include "CoreMinimal.h"
#include "InteractionRule.h"
#include "FireMaterialRule.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class MYPROJECT2_API UFireMaterialRule : public UInteractionRule
{
    GENERATED_BODY()

public:
    UFireMaterialRule();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
    float MinTemperatureToIgnite = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
    float BurnRateMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
    float CharredThreshold = 0.3f;

    virtual float EvaluateDamage(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const override;
    virtual EMaterialReactionType DetermineReaction(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const override;
    virtual bool CanApplyTo(const UMaterialDefinition* Material) const override;
};
