// StructuralLayer.h
#pragma once

#include "CoreMinimal.h"
#include "MaterialCore/MaterialTypes.h"
#include "MaterialInstanceState.h"
#include "StructuralLayer.generated.h"

class UMaterialDefinition;

USTRUCT(BlueprintType)
struct FStructuralLayer
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) EStructureLayerRole Role = EStructureLayerRole::None;
    UPROPERTY(BlueprintReadWrite) FName MaterialID;
    UPROPERTY(BlueprintReadWrite) EStructureLayerCondition Condition = EStructureLayerCondition::Pristine;

    UPROPERTY(BlueprintReadWrite) float Thickness = 0.2f;   // m
    UPROPERTY(BlueprintReadWrite) float LoadBearing = 0.0f; // 0-1, taşıma kapasitesi
    UPROPERTY(BlueprintReadWrite) float CurrentDamage = 0.0f;

    UPROPERTY(BlueprintReadWrite) FMaterialInstanceState InstanceState;

    TArray<TWeakObjectPtr<AActor>> LayerActors;

    void UpdateCondition();

    void ApplyLayerDamage(float Amount);

    bool IsStructurallySound() const
    {
        return Condition == EStructureLayerCondition::Pristine ||
               Condition == EStructureLayerCondition::Damaged;
    }
};
