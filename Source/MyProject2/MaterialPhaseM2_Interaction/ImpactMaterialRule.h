// ImpactMaterialRule.h
#pragma once
#include "MaterialCore/MaterialTypes.h"

#include "CoreMinimal.h"
#include "InteractionRule.h"
#include "ImpactMaterialRule.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class MYPROJECT2_API UImpactMaterialRule : public UInteractionRule
{
    GENERATED_BODY()

public:
    UImpactMaterialRule();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    float CrackThreshold = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    float ShatterThreshold = 0.7f;

    virtual float EvaluateDamage(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const override;
    virtual EMaterialReactionType DetermineReaction(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const override;
    virtual bool CanApplyTo(const UMaterialDefinition* Material) const override;
};
