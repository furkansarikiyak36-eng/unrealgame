// MaterialPropertyMapper.cpp
#include "MaterialPropertyMapper.h"
#include "MaterialCore/MaterialDefinition.h"
#include "Core/LogDefinition.h"
#include "StoneCore/StoneDefinition.h"
#include "StickCore/StickDefinition.h"

FMaterialProperties UMaterialPropertyMapper::MakeStoneProfile()
{
    FMaterialProperties P;
    P.Density = 2500.0f;
    P.Hardness = 80.0f;
    P.ImpactResistance = 70.0f;
    P.CompressionStrength = 90.0f;
    P.TensileStrength = 20.0f;
    P.Brittleness = 0.7f;
    P.Flammability = 0.0f;
    P.FuelValue = 0.0f;
    P.WaterAbsorption = 0.15f;
    P.WaterDamageRate = 0.02f;
    P.DecayRate = 0.001f;
    P.Workability = 0.3f;
    return P;
}

FMaterialProperties UMaterialPropertyMapper::MakeWoodProfile()
{
    FMaterialProperties P;
    P.Density = 600.0f;
    P.Hardness = 30.0f;
    P.ImpactResistance = 50.0f;
    P.CompressionStrength = 60.0f;
    P.TensileStrength = 70.0f;
    P.Brittleness = 0.3f;
    P.Flammability = 0.8f;
    P.FuelValue = 15.0f;
    P.WaterAbsorption = 0.4f;
    P.WaterDamageRate = 0.3f;
    P.DecayRate = 0.02f;
    P.Workability = 0.9f;
    return P;
}

FMaterialProperties UMaterialPropertyMapper::MakePlasterProfile()
{
    FMaterialProperties P;
    P.Density = 1000.0f;
    P.Hardness = 20.0f;
    P.ImpactResistance = 15.0f;
    P.CompressionStrength = 30.0f;
    P.TensileStrength = 10.0f;
    P.Brittleness = 0.9f;
    P.Flammability = 0.0f;
    P.FuelValue = 0.0f;
    P.WaterAbsorption = 0.8f;
    P.WaterDamageRate = 0.8f;
    P.DecayRate = 0.005f;
    P.Workability = 0.95f;
    return P;
}

FMaterialProperties UMaterialPropertyMapper::MakeSoilProfile()
{
    FMaterialProperties P;
    P.Density = 1500.0f;
    P.Hardness = 5.0f;
    P.ImpactResistance = 10.0f;
    P.CompressionStrength = 20.0f;
    P.TensileStrength = 5.0f;
    P.Brittleness = 0.1f;
    P.Flammability = 0.0f;
    P.FuelValue = 0.0f;
    P.WaterAbsorption = 0.9f;
    P.WaterDamageRate = 0.5f;
    P.DecayRate = 0.001f;
    P.Workability = 0.8f;
    return P;
}

FMaterialProperties UMaterialPropertyMapper::MakeMetalProfile()
{
    FMaterialProperties P;
    P.Density = 7800.0f;
    P.Hardness = 90.0f;
    P.ImpactResistance = 95.0f;
    P.CompressionStrength = 95.0f;
    P.TensileStrength = 95.0f;
    P.Brittleness = 0.1f;
    P.Flammability = 0.0f;
    P.FuelValue = 0.0f;
    P.WaterAbsorption = 0.0f;
    P.WaterDamageRate = 0.05f;
    P.DecayRate = 0.0001f;
    P.Workability = 0.4f;
    return P;
}

FMaterialProperties UMaterialPropertyMapper::FromLogDefinition(const ULogDefinition* LogDef)
{
    FMaterialProperties P = MakeWoodProfile();
    if (!LogDef) return P;
    if (UMaterialDefinition* Common = LogDef->CommonMaterialDefinition.LoadSynchronous()) return Common->Properties;

    P.Density = LogDef->Mass / FMath::Max(0.001f, LogDef->Volume);
    P.Flammability = LogDef->Flammability;
    P.CompressionStrength = LogDef->StructuralStrength * 0.5f;
    P.WaterAbsorption = 1.0f - LogDef->Buoyancy;
    return P;
}

FMaterialProperties UMaterialPropertyMapper::FromStoneDefinition(const UStoneDefinition* StoneDef)
{
    FMaterialProperties P = MakeStoneProfile();
    if (!StoneDef) return P;
    if (UMaterialDefinition* Common = StoneDef->CommonMaterialDefinition.LoadSynchronous()) return Common->Properties;

    P.CompressionStrength = StoneDef->CompressiveStrength * 0.2f;
    P.TensileStrength = StoneDef->TensileStrength;
    P.Hardness = FMath::Clamp(StoneDef->CompressiveStrength * 0.15f, 0.0f, 100.0f);
    return P;
}

FMaterialProperties UMaterialPropertyMapper::FromStickDefinition(const UStickDefinition* StickDef)
{
    FMaterialProperties P = MakeWoodProfile();
    if (!StickDef) return P;
    if (UMaterialDefinition* Common = StickDef->CommonMaterialDefinition.LoadSynchronous()) return Common->Properties;

    P.Flammability = StickDef->Flammability;
    P.WaterAbsorption = 1.0f - StickDef->Buoyancy;
    return P;
}
