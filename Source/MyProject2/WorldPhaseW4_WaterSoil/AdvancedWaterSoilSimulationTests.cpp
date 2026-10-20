#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WorldPhaseW4_WaterSoil/AdvancedWaterSoilSimulation.h"
#include "WorldCore/ChunkState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAdvancedHydrologyMassBalanceTest,
    "MyProject2.Hydrology.MassBalance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAdvancedHydrologyMassBalanceTest::RunTest(const FString& Parameters)
{
    UChunkState* Chunk = NewObject<UChunkState>();
    Chunk->Initialize(2);
    for (FChunkCell& Cell : Chunk->Cells)
    {
        Cell.SoilMoisture = 0.45f;
        Cell.Water.Volume = 0.04f;
        Cell.Terrain.Permeability = 0.65f;
    }

    UAdvancedWaterSoilSimulation* Model = NewObject<UAdvancedWaterSoilSimulation>();
    Model->CellSizeMeters = 4.f;
    const FHydrologyStepReport Report = Model->SimulateChunk(Chunk, 1.f, 18.f, 0.5f);

    TestTrue(TEXT("Water balance error remains bounded"), FMath::Abs(Report.WaterBalanceErrorMm) < 0.05f);
    TestTrue(TEXT("Infiltration cannot be negative"), Report.InfiltrationMm >= 0.f);
    TestTrue(TEXT("Evapotranspiration cannot be negative"), Report.EvapotranspirationMm >= 0.f);
    for (const FChunkCell& Cell : Chunk->Cells)
    {
        TestTrue(TEXT("Surface water cannot be negative"), Cell.Water.Volume >= 0.f);
        TestTrue(TEXT("Top layer storage cannot be negative"), Cell.Hydrology.TopLayerWaterMm >= 0.f);
        TestTrue(TEXT("Root layer storage cannot be negative"), Cell.Hydrology.RootLayerWaterMm >= 0.f);
        TestTrue(TEXT("Deep layer storage cannot be negative"), Cell.Hydrology.DeepLayerWaterMm >= 0.f);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAdvancedHydrologySnowpackTest,
    "MyProject2.Hydrology.SnowpackAndMelt",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAdvancedHydrologySnowpackTest::RunTest(const FString& Parameters)
{
    UChunkState* Chunk = NewObject<UChunkState>();
    Chunk->Initialize(2);
    UAdvancedWaterSoilSimulation* Model = NewObject<UAdvancedWaterSoilSimulation>();
    Model->CellSizeMeters = 4.f;

    Model->ApplyPrecipitation(Chunk, 10.f, true, -4.f);
    for (const FChunkCell& Cell : Chunk->Cells)
        TestTrue(TEXT("Snow precipitation enters snowpack"), FMath::IsNearlyEqual(Cell.Hydrology.SnowpackWaterEquivalentMm, 10.f));

    const FHydrologyStepReport Cold = Model->SimulateChunk(Chunk, 1.f, -3.f, 0.5f);
    TestTrue(TEXT("Snowpack does not melt below freezing"), FMath::IsNearlyEqual(Cold.SnowmeltMm, 0.f));

    const FHydrologyStepReport Warm = Model->SimulateChunk(Chunk, 1.f, 5.f, 0.5f);
    TestTrue(TEXT("Warming melts accumulated snow"), Warm.SnowmeltMm > 0.f);
    for (const FChunkCell& Cell : Chunk->Cells)
        TestTrue(TEXT("Snowpack storage never becomes negative"), Cell.Hydrology.SnowpackWaterEquivalentMm >= 0.f);
    return true;
}
#endif
