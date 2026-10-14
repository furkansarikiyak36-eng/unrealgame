// MaterialTestHarness.cpp
#include "MaterialTestHarness.h"
#include "MaterialCore/MaterialDefinition.h"
#include "MaterialCore/MaterialProperties.h"
#include "MaterialPhaseM1_Definition/MaterialPropertyMapper.h"
#include "MaterialPhaseM2_Interaction/FireMaterialRule.h"
#include "MaterialPhaseM2_Interaction/WaterMaterialRule.h"
#include "MaterialPhaseM2_Interaction/ImpactMaterialRule.h"
#include "MaterialPhaseM2_Interaction/TimeMaterialRule.h"
#include "MaterialPhaseM4_Composite/StructuralLayer.h"
#include "MaterialPhaseM4_Composite/LayerDamageResolver.h"

bool UMaterialTestHarness::TestStoneProperties()
{
    const FMaterialProperties P = UMaterialPropertyMapper::MakeStoneProfile();
    return P.Flammability < 0.1f && P.CompressionStrength > 50.0f && P.Density > 2000.0f;
}

bool UMaterialTestHarness::TestWoodProperties()
{
    const FMaterialProperties P = UMaterialPropertyMapper::MakeWoodProfile();
    return P.Flammability > 0.5f && P.DecayRate > 0.01f && P.WaterAbsorption > 0.0f;
}

bool UMaterialTestHarness::TestPlasterProperties()
{
    const FMaterialProperties P = UMaterialPropertyMapper::MakePlasterProfile();
    return P.WaterDamageRate > 0.5f && P.Flammability < 0.1f && P.Brittleness > 0.5f;
}

bool UMaterialTestHarness::TestFireRule()
{
    UFireMaterialRule* Rule = NewObject<UFireMaterialRule>();
    UMaterialDefinition* Wood = NewObject<UMaterialDefinition>();
    UMaterialDefinition* Stone = NewObject<UMaterialDefinition>();
    if (!Rule || !Wood || !Stone) return false;
    Wood->MaterialClass = EMaterialClass::Wood;
    Wood->Properties = UMaterialPropertyMapper::MakeWoodProfile();
    Stone->MaterialClass = EMaterialClass::Stone;
    Stone->Properties = UMaterialPropertyMapper::MakeStoneProfile();
    FMaterialInteractionContext Context;
    Context.AmbientTemperature = 400.0f;
    Context.DurationHours = 24.0f;
    return Rule->EvaluateDamage(Wood, Context) > 0.0f && FMath::IsNearlyZero(Rule->EvaluateDamage(Stone, Context));
}

bool UMaterialTestHarness::TestWaterRule()
{
    UWaterMaterialRule* Rule = NewObject<UWaterMaterialRule>();
    UMaterialDefinition* Plaster = NewObject<UMaterialDefinition>();
    if (!Rule || !Plaster) return false;
    Plaster->MaterialClass = EMaterialClass::Plaster;
    Plaster->Properties = UMaterialPropertyMapper::MakePlasterProfile();
    FMaterialInteractionContext Context;
    Context.Moisture = 0.8f;
    Context.DurationHours = 24.0f;
    return Rule->EvaluateDamage(Plaster, Context) > 0.0f;
}

bool UMaterialTestHarness::TestImpactRule()
{
    UImpactMaterialRule* Rule = NewObject<UImpactMaterialRule>();
    UMaterialDefinition* Stone = NewObject<UMaterialDefinition>();
    if (!Rule || !Stone) return false;
    Stone->MaterialClass = EMaterialClass::Stone;
    Stone->Properties = UMaterialPropertyMapper::MakeStoneProfile();
    FMaterialInteractionContext Context;
    Context.Magnitude = 100.0f;
    return Rule->EvaluateDamage(Stone, Context) > 0.0f && Rule->DetermineReaction(Stone, Context) != EMaterialReactionType::None;
}

bool UMaterialTestHarness::TestTimeRule()
{
    UTimeMaterialRule* Rule = NewObject<UTimeMaterialRule>();
    UMaterialDefinition* Wood = NewObject<UMaterialDefinition>();
    if (!Rule || !Wood) return false;
    Wood->MaterialClass = EMaterialClass::Wood;
    Wood->Properties = UMaterialPropertyMapper::MakeWoodProfile();
    FMaterialInteractionContext Context;
    Context.Moisture = 0.75f;
    Context.AmbientTemperature = 24.0f;
    Context.DurationHours = 24.0f;
    return Rule->EvaluateDamage(Wood, Context) > 0.0f && Rule->DetermineReaction(Wood, Context) == EMaterialReactionType::Rot;
}

bool UMaterialTestHarness::TestCompositeStructure()
{
    FStructuralLayer Layer;
    Layer.Role = EStructureLayerRole::Skeleton;
    Layer.LoadBearing = 1.0f;
    Layer.ApplyLayerDamage(1.0f);
    return Layer.Condition == EStructureLayerCondition::Collapsed && FMath::IsNearlyZero(Layer.LoadBearing);
}

bool UMaterialTestHarness::TestLayerDamagePropagation()
{
    return ULayerDamageResolver::ShouldDamagePropagateToSkeleton(EStructureLayerCondition::Collapsed) &&
        !ULayerDamageResolver::ShouldDamagePropagateToSkeleton(EStructureLayerCondition::Damaged);
}

bool UMaterialTestHarness::RunAllTests(FString& OutReport)
{
    OutReport.Reset();
    int32 Passed = 0;
    int32 Total = 0;
    auto Run = [&OutReport, &Passed, &Total](const TCHAR* Name, bool(*Fn)())
    {
        ++Total;
        const bool bResult = Fn();
        if (bResult) ++Passed;
        OutReport += FString::Printf(TEXT("%s: %s\n"), Name, bResult ? TEXT("PASS") : TEXT("FAIL"));
    };
    Run(TEXT("StoneProperties"), TestStoneProperties);
    Run(TEXT("WoodProperties"), TestWoodProperties);
    Run(TEXT("PlasterProperties"), TestPlasterProperties);
    Run(TEXT("FireRule"), TestFireRule);
    Run(TEXT("WaterRule"), TestWaterRule);
    Run(TEXT("ImpactRule"), TestImpactRule);
    Run(TEXT("TimeRule"), TestTimeRule);
    Run(TEXT("CompositeStructure"), TestCompositeStructure);
    Run(TEXT("LayerDamagePropagation"), TestLayerDamagePropagation);
    OutReport += FString::Printf(TEXT("\nTotal: %d/%d\n"), Passed, Total);
    return Passed == Total;
}
