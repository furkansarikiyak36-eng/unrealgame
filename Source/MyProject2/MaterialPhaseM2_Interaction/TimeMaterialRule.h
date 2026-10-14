// TimeMaterialRule.h
#pragma once
#include "MaterialCore/MaterialTypes.h"

#include "CoreMinimal.h"
#include "InteractionRule.h"
#include "TimeMaterialRule.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class MYPROJECT2_API UTimeMaterialRule : public UInteractionRule
{
    GENERATED_BODY()

public:
    UTimeMaterialRule();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
    float MinDecayRate = 0.001f;

    virtual float EvaluateDamage(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const override;
    virtual EMaterialReactionType DetermineReaction(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const override;
    virtual bool CanApplyTo(const UMaterialDefinition* Material) const override;
};
