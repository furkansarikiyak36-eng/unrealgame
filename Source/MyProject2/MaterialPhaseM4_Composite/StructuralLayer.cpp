// StructuralLayer.cpp
#include "StructuralLayer.h"

void FStructuralLayer::UpdateCondition()
{
    if (CurrentDamage < 0.2f)      Condition = EStructureLayerCondition::Pristine;
    else if (CurrentDamage < 0.5f) Condition = EStructureLayerCondition::Damaged;
    else if (CurrentDamage < 0.75f)Condition = EStructureLayerCondition::Weakened;
    else if (CurrentDamage < 0.95f)Condition = EStructureLayerCondition::Failing;
    else                           Condition = EStructureLayerCondition::Collapsed;
}

void FStructuralLayer::ApplyLayerDamage(float Amount)
{
    CurrentDamage = FMath::Clamp(CurrentDamage + Amount, 0.0f, 1.0f);
    LoadBearing = FMath::Clamp(1.0f - CurrentDamage, 0.0f, 1.0f);
    UpdateCondition();
}
