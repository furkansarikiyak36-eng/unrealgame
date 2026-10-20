#include "CropFieldActor.h"
#include "Engine/World.h"
#include "PlantGrowthComponent.h"
#include "PlantStageComponent.h"
#include "PlantVisualComponent.h"
#include "PlantDefinition.h"
#include "PhaseT3_Soil/SoilCellComponent.h"
#include "PhaseT3_Soil/SoilMoistureComponent.h"
#include "PhaseT3_Soil/SoilNutrientComponent.h"
#include "PhaseT3_Soil/SoilPHComponent.h"
#include "Components/StaticMeshComponent.h"
#include "SimCore/SimulationSubsystem.h"
#include "SimCore/SimulationState.h"
#include "RegionSeason/RegionSeasonSubsystem.h"
#include "RegionSeason/RegionSeasonTypes.h"

ACropFieldActor::ACropFieldActor()
{
    PrimaryActorTick.bCanEverTick = false;
    FieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Field"));
    RootComponent = FieldMesh;
    GrowthComponent = CreateDefaultSubobject<UPlantGrowthComponent>(TEXT("Growth"));
    StageComponent = CreateDefaultSubobject<UPlantStageComponent>(TEXT("Stage"));
    VisualComponent = CreateDefaultSubobject<UPlantVisualComponent>(TEXT("Visual"));
    SoilComponent = CreateDefaultSubobject<USoilCellComponent>(TEXT("Soil"));
    CreateDefaultSubobject<USoilMoistureComponent>(TEXT("SoilMoisture"));
    CreateDefaultSubobject<USoilNutrientComponent>(TEXT("SoilNutrients"));
    CreateDefaultSubobject<USoilPHComponent>(TEXT("SoilPH"));
}

void ACropFieldActor::BeginPlay()
{
    Super::BeginPlay();
    if (USimulationSubsystem* Sim = GetWorld()->GetSubsystem<USimulationSubsystem>())
        Sim->OnDayTick.AddDynamic(this, &ACropFieldActor::HandleSimulationDay);
    if (StageComponent)
        StageComponent->OnStageChanged.AddDynamic(this, &ACropFieldActor::HandleStageChanged);
}

void ACropFieldActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld())
        if (USimulationSubsystem* Sim = GetWorld()->GetSubsystem<USimulationSubsystem>())
            Sim->OnDayTick.RemoveDynamic(this, &ACropFieldActor::HandleSimulationDay);
    Super::EndPlay(EndPlayReason);
}

void ACropFieldActor::PlantCrop(UPlantDefinition* Def, int32 Count)
{
    if (!Def || !GrowthComponent) return;
    GrowthComponent->SetDefinition(Def);
    GrowthComponent->PlantState.SeedCount = FMath::Max(0, Count);
    if (VisualComponent) VisualComponent->SetDefinition(Def);
    if (StageComponent) StageComponent->CheckStageTransition(EGrowthStage::Seed);
}

void ACropFieldActor::HandleSimulationDay(int32)
{
    UpdateDaily();
}

void ACropFieldActor::UpdateDaily()
{
    if (!GetWorld() || !GrowthComponent || !SoilComponent || !GrowthComponent->Definition) return;

    USimulationSubsystem* Sim = GetWorld()->GetSubsystem<USimulationSubsystem>();
    USimulationState* SimState = Sim ? Sim->GetState() : nullptr;
    if (!SimState) return;

    FSoilState& Soil = SoilComponent->GetMutableSoilState();
    EWorldSeason Season = EWorldSeason::Spring;
    EWorldRegionType RegionType = EWorldRegionType::Unknown;
    bool bHasKnownRegion = false;
    float DaylightHours = 12.f;
    float SeasonalTemperatureOffset = 0.f;
    float SeasonalPrecipitationMultiplier = 1.f;

    if (URegionSeasonSubsystem* RegionSeason = GetWorld()->GetSubsystem<URegionSeasonSubsystem>())
    {
        const FRegionClimateSnapshot Climate = RegionSeason->GetClimateAtLocation(GetActorLocation());
        Season = Climate.Season;
        RegionType = Climate.RegionType;
        bHasKnownRegion = Climate.bHasKnownRegion;
        DaylightHours = Climate.DaylightHours;
        SeasonalTemperatureOffset = Climate.TemperatureOffsetC;
        SeasonalPrecipitationMultiplier = Climate.PrecipitationMultiplier;
    }

    const float TempMax = SimState->TempMax + SeasonalTemperatureOffset;
    const float TempMin = SimState->TempMin + SeasonalTemperatureOffset;
    const float Rainfall = FMath::Max(0.f, SimState->Rainfall * SeasonalPrecipitationMultiplier);

    float Evaporation = 2.f;
    if (USoilMoistureComponent* Moisture = FindComponentByClass<USoilMoistureComponent>())
    {
        const float EstimatedHumidity = Rainfall > 0.f ? 0.8f : 0.45f;
        Evaporation = Moisture->ComputeEvaporation((TempMax + TempMin) * 0.5f, EstimatedHumidity);
    }

    // Update soil water first; plant transpiration and nutrient uptake are applied by the crop model below.
    const float ExcessWater = FMath::Max(0.f, Soil.Water + Rainfall - Soil.WaterMax);
    const float Runoff = ExcessWater * (Soil.Texture == ESoilTexture::Sandy ? 0.25f : Soil.Texture == ESoilTexture::Clay ? 0.65f : 0.4f);
    const float Drainage = ExcessWater - Runoff;
    SoilComponent->UpdateDaily(Rainfall, 0.f, Evaporation, 0.f, Runoff, Drainage);

    if (USoilNutrientComponent* Nutrients = FindComponentByClass<USoilNutrientComponent>())
        Nutrients->ApplyDailyLeaching();

    GrowthComponent->UpdateDailyAdvanced(
        TempMax, TempMin, Rainfall, DaylightHours, Season, RegionType,
        bHasKnownRegion, Soil, 1.f);

    if (StageComponent)
        StageComponent->CheckStageTransition(GrowthComponent->GetCurrentStage());
    if (VisualComponent)
        VisualComponent->UpdateVisual(GrowthComponent->GetCurrentStage(), GrowthComponent->GetState().StageProgress);
}

void ACropFieldActor::HandleStageChanged(EGrowthStage, EGrowthStage NewStage)
{
    if (VisualComponent && GrowthComponent)
        VisualComponent->UpdateVisual(NewStage, GrowthComponent->GetState().StageProgress);
}