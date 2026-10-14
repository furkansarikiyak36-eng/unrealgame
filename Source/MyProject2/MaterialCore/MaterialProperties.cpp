// MaterialProperties.cpp
#include "MaterialProperties.h"

float FMaterialProperties::GetProperty(EMaterialPropertyKind Property) const
{
    switch (Property)
    {
        case EMaterialPropertyKind::Density:             return Density;
        case EMaterialPropertyKind::Hardness:            return Hardness;
        case EMaterialPropertyKind::ImpactResistance:    return ImpactResistance;
        case EMaterialPropertyKind::CompressionStrength: return CompressionStrength;
        case EMaterialPropertyKind::TensileStrength:     return TensileStrength;
        case EMaterialPropertyKind::Brittleness:         return Brittleness;
        case EMaterialPropertyKind::Flammability:        return Flammability;
        case EMaterialPropertyKind::FuelValue:           return FuelValue;
        case EMaterialPropertyKind::WaterAbsorption:     return WaterAbsorption;
        case EMaterialPropertyKind::WaterDamageRate:     return WaterDamageRate;
        case EMaterialPropertyKind::DecayRate:           return DecayRate;
        case EMaterialPropertyKind::Workability:         return Workability;
    }
    return 0.0f;
}

void FMaterialProperties::SetProperty(EMaterialPropertyKind Property, float Value)
{
    switch (Property)
    {
        case EMaterialPropertyKind::Density:             Density = Value; break;
        case EMaterialPropertyKind::Hardness:            Hardness = Value; break;
        case EMaterialPropertyKind::ImpactResistance:    ImpactResistance = Value; break;
        case EMaterialPropertyKind::CompressionStrength: CompressionStrength = Value; break;
        case EMaterialPropertyKind::TensileStrength:     TensileStrength = Value; break;
        case EMaterialPropertyKind::Brittleness:         Brittleness = Value; break;
        case EMaterialPropertyKind::Flammability:        Flammability = Value; break;
        case EMaterialPropertyKind::FuelValue:           FuelValue = Value; break;
        case EMaterialPropertyKind::WaterAbsorption:     WaterAbsorption = Value; break;
        case EMaterialPropertyKind::WaterDamageRate:     WaterDamageRate = Value; break;
        case EMaterialPropertyKind::DecayRate:           DecayRate = Value; break;
        case EMaterialPropertyKind::Workability:         Workability = Value; break;
    }
}

float FMaterialProperties::CompareHardness(const FMaterialProperties& Other) const
{
    if (Other.Hardness <= 0.0f) return 1.0f;
    return FMath::Clamp(Hardness / Other.Hardness, 0.0f, 10.0f);
}
