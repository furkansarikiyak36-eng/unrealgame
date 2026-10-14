// MaterialDefinition.cpp
#include "MaterialDefinition.h"

bool UMaterialDefinition::ValidateDefinition(FString& OutError) const
{
    OutError.Reset();
    if (MaterialID.IsNone()) { OutError = TEXT("MaterialID boş"); return false; }
    if (Properties.Density <= 0.0f) { OutError = TEXT("Density <= 0"); return false; }
    if (Properties.Flammability < 0.0f || Properties.Flammability > 1.0f)
    { OutError = TEXT("Flammability 0-1 dışı"); return false; }
    if (Properties.WaterAbsorption < 0.0f || Properties.WaterAbsorption > 1.0f)
    { OutError = TEXT("WaterAbsorption 0-1 dışı"); return false; }
    const auto InUnitRange = [](float Value) { return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f; };
    if (!InUnitRange(Properties.Brittleness)) { OutError = TEXT("Brittleness 0-1 dışı"); return false; }
    if (!InUnitRange(Properties.WaterDamageRate)) { OutError = TEXT("WaterDamageRate 0-1 dışı"); return false; }
    if (!InUnitRange(Properties.DecayRate)) { OutError = TEXT("DecayRate 0-1 dışı"); return false; }
    if (!InUnitRange(Properties.Workability)) { OutError = TEXT("Workability 0-1 dışı"); return false; }
    if (!FMath::IsFinite(Properties.Hardness) || Properties.Hardness < 0.0f ||
        !FMath::IsFinite(Properties.ImpactResistance) || Properties.ImpactResistance < 0.0f ||
        !FMath::IsFinite(Properties.CompressionStrength) || Properties.CompressionStrength < 0.0f ||
        !FMath::IsFinite(Properties.TensileStrength) || Properties.TensileStrength < 0.0f ||
        !FMath::IsFinite(Properties.FuelValue) || Properties.FuelValue < 0.0f)
    { OutError = TEXT("Negatif veya sonlu olmayan malzeme parametresi"); return false; }
    return true;
}

FMaterialReactionProfile UMaterialDefinition::GetReactionFor(EMaterialPropertyKind Property) const
{
    switch (Property)
    {
        case EMaterialPropertyKind::Flammability: return FireReaction;
        case EMaterialPropertyKind::WaterDamageRate: return WaterReaction;
        case EMaterialPropertyKind::DecayRate: return TimeReaction;
        default: return FMaterialReactionProfile();
    }
}

FMaterialReactionProfile UMaterialDefinition::GetImpactReactionForDamage(float DamageAmount) const
{
    // Hasar şiddetine göre tepki seç
    if (DamageAmount < 10.0f)
    {
        if (const FMaterialReactionProfile* Found = ImpactReactions.Find(TEXT("Light")))
            return *Found;
    }
    else if (DamageAmount < 50.0f)
    {
        if (const FMaterialReactionProfile* Found = ImpactReactions.Find(TEXT("Medium")))
            return *Found;
    }
    else
    {
        if (const FMaterialReactionProfile* Found = ImpactReactions.Find(TEXT("Heavy")))
            return *Found;
    }
    return FMaterialReactionProfile();
}
