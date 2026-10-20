#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WorldPhaseW4_WaterSoil/AdvancedWaterSoilSimulation.h"
#include "WorldCore/ChunkState.h"
#include "WorldPhaseW6_Persistence/ChunkSerializer.h"

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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAdvancedHydrologyPersistenceTest,
    "MyProject2.Hydrology.PersistenceRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAdvancedHydrologyPersistenceTest::RunTest(const FString& Parameters)
{
    UChunkState* Source = NewObject<UChunkState>();
    Source->Initialize(2);
    FChunkCell& Cell = Source->Cells[0];
    Cell.Hydrology.bInitialized = true;
    Cell.Hydrology.TopLayerWaterMm = 21.5f;
    Cell.Hydrology.RootLayerWaterMm = 67.f;
    Cell.Hydrology.DeepLayerWaterMm = 118.f;
    Cell.Hydrology.SnowpackWaterEquivalentMm = 14.f;
    Cell.Hydrology.SoilTemperatureC = -2.5f;
    Cell.Hydrology.CumulativeErosionIndex = 3.4f;
    Cell.Terrain.SoilTexture = EWorldSoilTexture::Clay;
    Cell.Terrain.Compaction = 0.42f;
    Cell.Terrain.OrganicMatterFraction = 0.08f;
    Cell.Terrain.BedrockDepthMeters = 0.9f;
    Cell.Terrain.RootingDepthMeters = 0.35f;
    Cell.Terrain.SoilDepthMeters = 0.42f;
    Cell.Terrain.ExcavatedDepthMeters = 1.08f;
    Cell.Terrain.GroundTreatment = EWorldGroundTreatment::Pit;

    const FSerializedChunk Saved = UChunkSerializer::SerializeChunk(Source);
    UChunkState* Restored = UChunkSerializer::DeserializeChunk(GetTransientPackage(), Saved);
    TestNotNull(TEXT("Chunk deserializes"), Restored);
    if (!Restored) return false;

    const FChunkCell& R = Restored->Cells[0];
    TestTrue(TEXT("Root-zone storage persists"), FMath::IsNearlyEqual(R.Hydrology.RootLayerWaterMm, 67.f));
    TestTrue(TEXT("Snowpack persists"), FMath::IsNearlyEqual(R.Hydrology.SnowpackWaterEquivalentMm, 14.f));
    TestTrue(TEXT("Soil temperature persists"), FMath::IsNearlyEqual(R.Hydrology.SoilTemperatureC, -2.5f));
    TestTrue(TEXT("Soil texture persists"), R.Terrain.SoilTexture == EWorldSoilTexture::Clay);
    TestTrue(TEXT("Compaction persists"), FMath::IsNearlyEqual(R.Terrain.Compaction, 0.42f));
    TestTrue(TEXT("Bedrock depth persists"), FMath::IsNearlyEqual(R.Terrain.BedrockDepthMeters, 0.9f));
    TestTrue(TEXT("Remaining soil depth persists"), FMath::IsNearlyEqual(R.Terrain.SoilDepthMeters, 0.42f));
    TestTrue(TEXT("Excavation depth persists"), FMath::IsNearlyEqual(R.Terrain.ExcavatedDepthMeters, 1.08f));
    TestTrue(TEXT("Ground treatment persists"), R.Terrain.GroundTreatment == EWorldGroundTreatment::Pit);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAdvancedHydrologyTextureResponseTest,
    "MyProject2.Hydrology.SoilTextureResponse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAdvancedHydrologyTextureResponseTest::RunTest(const FString& Parameters)
{
    UChunkState* SandChunk = NewObject<UChunkState>();
    UChunkState* ClayChunk = NewObject<UChunkState>();
    SandChunk->Initialize(2);
    ClayChunk->Initialize(2);
    for (FChunkCell& Cell : SandChunk->Cells)
    {
        Cell.SoilMoisture = 0.3f;
        Cell.Terrain.Permeability = 0.5f;
        Cell.Terrain.SoilTexture = EWorldSoilTexture::Sand;
        Cell.Water.Volume = 0.1f;
    }
    for (FChunkCell& Cell : ClayChunk->Cells)
    {
        Cell.SoilMoisture = 0.3f;
        Cell.Terrain.Permeability = 0.5f;
        Cell.Terrain.SoilTexture = EWorldSoilTexture::Clay;
        Cell.Water.Volume = 0.1f;
    }
    UAdvancedWaterSoilSimulation* Model = NewObject<UAdvancedWaterSoilSimulation>();
    Model->CellSizeMeters = 4.f;
    const FHydrologyStepReport Sand = Model->SimulateChunk(SandChunk, 1.f, 18.f, 0.5f);
    const FHydrologyStepReport Clay = Model->SimulateChunk(ClayChunk, 1.f, 18.f, 0.5f);
    TestTrue(TEXT("Sandy soil infiltrates more rapidly than clay under identical conditions"), Sand.InfiltrationMm > Clay.InfiltrationMm);
    return true;
}

#endif
