// WaterMaterialRule.h
#pragma once
#include "MaterialCore/MaterialTypes.h"

#include "CoreMinimal.h"
#include "InteractionRule.h"
#include "WaterMaterialRule.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class MYPROJECT2_API UWaterMaterialRule : public UInteractionRule
{
    GENERATED_BODY()

public:
    UWaterMaterialRule();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
    float MinWaterContact = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
    float SwellThreshold = 0.5f;

    virtual float EvaluateDamage(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const override;
    virtual EMaterialReactionType DetermineReaction(const UMaterialDefinition* Material, const FMaterialInteractionContext& Context) const override;
    virtual bool CanApplyTo(const UMaterialDefinition* Material) const override;
};
