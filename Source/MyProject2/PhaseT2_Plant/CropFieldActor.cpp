#include "CropFieldActor.h"
#include "Engine/World.h"
#include "PlantGrowthComponent.h"
#include "PlantStageComponent.h"
#include "PlantVisualComponent.h"
#include "PlantDefinition.h"
#include "PhaseT3_Soil/SoilCellComponent.h"
#include "PhaseT5_Harvest/HarvestComponent.h"
#include "PhaseT6_Animal/FeedStorageComponent.h"
#include "InventoryCore/InventoryComponent.h"
#include "WorldCore/WorldStateSubsystem.h"
#include "WorldCore/ChunkState.h"
#include "PhaseT3_Soil/SoilMoistureComponent.h"
#include "PhaseT3_Soil/SoilNutrientComponent.h"
#include "PhaseT3_Soil/SoilPHComponent.h"
#include "Components/StaticMeshComponent.h"
#include "SimCore/SimulationSubsystem.h"
#include "SimCore/SimulationState.h"
#include "RegionSeason/RegionSeasonSubsystem.h"
#include "RegionSeason/RegionSeasonTypes.h"
ACropFieldActor::ACropFieldActor(){PrimaryActorTick.bCanEverTick=false;FieldMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Field"));RootComponent=FieldMesh;GrowthComponent=CreateDefaultSubobject<UPlantGrowthComponent>(TEXT("Growth"));StageComponent=CreateDefaultSubobject<UPlantStageComponent>(TEXT("Stage"));VisualComponent=CreateDefaultSubobject<UPlantVisualComponent>(TEXT("Visual"));SoilComponent=CreateDefaultSubobject<USoilCellComponent>(TEXT("Soil"));HarvestComponent=CreateDefaultSubobject<UHarvestComponent>(TEXT("Harvest"));CreateDefaultSubobject<USoilMoistureComponent>(TEXT("SoilMoisture"));CreateDefaultSubobject<USoilNutrientComponent>(TEXT("SoilNutrients"));CreateDefaultSubobject<USoilPHComponent>(TEXT("SoilPH"));}
void ACropFieldActor::BeginPlay(){Super::BeginPlay();if(USimulationSubsystem* Sim=GetWorld()->GetSubsystem<USimulationSubsystem>())Sim->OnDayTick.AddDynamic(this,&ACropFieldActor::HandleSimulationDay);if(StageComponent)StageComponent->OnStageChanged.AddDynamic(this,&ACropFieldActor::HandleStageChanged);}
void ACropFieldActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld())
        if (USimulationSubsystem* Sim = GetWorld()->GetSubsystem<USimulationSubsystem>())
            Sim->OnDayTick.RemoveDynamic(this, &ACropFieldActor::HandleSimulationDay);
    Super::EndPlay(EndPlayReason);
}
void ACropFieldActor::PlantCrop(UPlantDefinition* Def,int32 Count)
{
 if(!Def||!GrowthComponent||Count<=0||bCropPlanted)return;
 FString Error;if(!Def->ValidateDefinition(Error)){UE_LOG(LogTemp,Warning,TEXT("Crop planting rejected: %s"),*Error);return;}
 GrowthComponent->SetDefinition(Def);GrowthComponent->PlantState.SeedCount=Count;bCropPlanted=true;
 if(VisualComponent)VisualComponent->SetDefinition(Def);
 if(StageComponent)StageComponent->CheckStageTransition(EGrowthStage::Seed);
 if(UWorldStateSubsystem* WS=GetWorld()?GetWorld()->GetSubsystem<UWorldStateSubsystem>():nullptr)
   if(UChunkState* Chunk=WS->GetOrCreateChunkAtWorld(GetActorLocation()))
      if(Chunk->Metadata.ActivationMode==EChunkActivationMode::Passive)Chunk->Metadata.ActivationMode=EChunkActivationMode::Scheduled;
}
bool ACropFieldActor::PlantCropFromInventory(UPlantDefinition* Def,UInventoryComponent* Inventory,FName SeedItemID,int32 SeedCount)
{
 if(!Def||!Inventory||SeedItemID.IsNone()||SeedCount<=0||!GrowthComponent||bCropPlanted||!Inventory->HasItem(SeedItemID,SeedCount))return false;
 FString Error;if(!Def->ValidateDefinition(Error))return false;
 if(Inventory->RemoveItem(SeedItemID,SeedCount)!=EInventoryResult::Success)return false;
 PlantCrop(Def,SeedCount);
 return GrowthComponent->GetDefinition()==Def&&GrowthComponent->PlantState.SeedCount==SeedCount;
}
float ACropFieldActor::HarvestToFeedStorage(UFeedStorageComponent* Destination,FName FeedID)
{
 if(!Destination||FeedID.IsNone()||!HarvestComponent||!GrowthComponent||!HarvestComponent->CanHarvest())return 0.0f;
 UPlantDefinition* Def=GrowthComponent->GetDefinition();
 const float HarvestIndex=Def?FMath::Clamp(Def->HarvestIndex,0.0f,1.0f):FMath::Clamp(HarvestComponent->HarvestIndex,0.0f,1.0f);
 const float ExpectedYield=FMath::Max(0.0f,GrowthComponent->GetState().Biomass)*HarvestIndex;
 if(!FMath::IsFinite(ExpectedYield)||ExpectedYield<=KINDA_SMALL_NUMBER||!Destination->CanAddFeed(FeedID,ExpectedYield))return 0.0f;
 FFeedItem Feed;Feed.FeedID=FeedID;Feed.DryMatter=0.85f;Feed.MetabolizableEnergy=10.0f;Feed.CrudeProtein=0.15f;
 if(!Destination->TryAddFeed(FeedID,ExpectedYield,Feed))return 0.0f;
 const float ActualYield=HarvestComponent->Harvest();
 if(!FMath::IsNearlyEqual(ActualYield,ExpectedYield,0.01f))
 {
   Destination->ConsumeFeed(FeedID,ExpectedYield);
   return 0.0f;
 }
 bCropPlanted=false;
 return ActualYield;
}
void ACropFieldActor::HandleSimulationDay(int32){UpdateDaily();}
void ACropFieldActor::UpdateDaily()
{
    if (!GetWorld() || !GrowthComponent || !SoilComponent || !GrowthComponent->Definition) return;
    USimulationSubsystem* Sim = GetWorld()->GetSubsystem<USimulationSubsystem>();
    USimulationState* SimState = Sim ? Sim->GetState() : nullptr;
    if (!SimState) return;

    // Region and season: temperature offset, rain multiplier, day length.
    EWorldSeason Season = EWorldSeason::Spring;
    EWorldRegionType RegionType = EWorldRegionType::Unknown;
    bool bHasKnownRegion = false;
    float DaylightHours = 12.f, SeasonalTemperatureOffset = 0.f, SeasonalPrecipitationMultiplier = 1.f;
    if (URegionSeasonSubsystem* RegionSeason = GetWorld()->GetSubsystem<URegionSeasonSubsystem>())
    {
        const FRegionClimateSnapshot Climate = RegionSeason->GetClimateAtLocation(GetActorLocation());
        Season = Climate.Season; RegionType = Climate.RegionType; bHasKnownRegion = Climate.bHasKnownRegion;
        DaylightHours = Climate.DaylightHours; SeasonalTemperatureOffset = Climate.TemperatureOffsetC;
        SeasonalPrecipitationMultiplier = Climate.PrecipitationMultiplier;
    }
    const float TempMax = SimState->TempMax + SeasonalTemperatureOffset;
    const float TempMin = SimState->TempMin + SeasonalTemperatureOffset;
    const float Rainfall = FMath::Max(0.f, SimState->Rainfall * SeasonalPrecipitationMultiplier);

    FSoilState& Soil = SoilComponent->GetMutableSoilState();
    UWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UWorldStateSubsystem>();
    FChunkCell* SharedCell = WorldState ? WorldState->GetCellAtWorld(GetActorLocation()) : nullptr;
    if (SharedCell)
    {
        // The world cell owns water and nutrients (its hydrology already took the rain); the field reads them.
        Soil.Water = FMath::Clamp(SharedCell->SoilMoisture, 0.0f, 1.0f) * FMath::Max(1.0f, Soil.WaterMax);
        Soil.Nitrogen = SharedCell->SoilNitrogen; Soil.Phosphorus = SharedCell->SoilPhosphorus;
        Soil.Potassium = SharedCell->SoilPotassium; Soil.pH = SharedCell->SoilPH;
        if (UChunkState* Chunk = WorldState->GetOrCreateChunkAtWorld(GetActorLocation()))
            if (Chunk->Metadata.ActivationMode == EChunkActivationMode::Passive) Chunk->Metadata.ActivationMode = EChunkActivationMode::Scheduled;
    }
    else
    {
        // No world cell: the field keeps its own little water balance.
        float Evaporation = 2.f;
        if (USoilMoistureComponent* Moisture = FindComponentByClass<USoilMoistureComponent>())
            Evaporation = Moisture->ComputeEvaporation((TempMax + TempMin) * 0.5f, Rainfall > 0.f ? 0.8f : 0.45f);
        const float ExcessWater = FMath::Max(0.f, Soil.Water + Rainfall - Soil.WaterMax);
        const float Runoff = ExcessWater * (Soil.Texture == ESoilTexture::Sandy ? 0.25f : Soil.Texture == ESoilTexture::Clay ? 0.65f : 0.4f);
        SoilComponent->UpdateDaily(Rainfall, 0.f, Evaporation, 0.f, Runoff, ExcessWater - Runoff);
        if (USoilNutrientComponent* Nutrients = FindComponentByClass<USoilNutrientComponent>()) Nutrients->ApplyDailyLeaching();
    }

    // Growth takes its water and nutrients out of Soil.
    GrowthComponent->UpdateDailyAdvanced(TempMax, TempMin, Rainfall, DaylightHours, Season, RegionType, bHasKnownRegion, Soil, 1.f);

    if (SharedCell)
    {
        SharedCell->SoilMoisture = FMath::Clamp(Soil.Water / FMath::Max(1.0f, Soil.WaterMax), 0.0f, 1.0f);
        SharedCell->SoilNitrogen = Soil.Nitrogen; SharedCell->SoilPhosphorus = Soil.Phosphorus; SharedCell->SoilPotassium = Soil.Potassium;
        SharedCell->bIsDirty = true;
        if (UChunkState* Chunk = WorldState->GetOrCreateChunkAtWorld(GetActorLocation())) Chunk->MarkDirty();
    }
    if (StageComponent) StageComponent->CheckStageTransition(GrowthComponent->GetCurrentStage());
    if (VisualComponent) VisualComponent->UpdateVisual(GrowthComponent->GetCurrentStage(), GrowthComponent->GetState().StageProgress);
}
void ACropFieldActor::HandleStageChanged(EGrowthStage,EGrowthStage NewStage){if(VisualComponent&&GrowthComponent)VisualComponent->UpdateVisual(NewStage,GrowthComponent->GetState().StageProgress);}
