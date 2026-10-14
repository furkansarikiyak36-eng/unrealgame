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
ACropFieldActor::ACropFieldActor(){PrimaryActorTick.bCanEverTick=false;FieldMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Field"));RootComponent=FieldMesh;GrowthComponent=CreateDefaultSubobject<UPlantGrowthComponent>(TEXT("Growth"));StageComponent=CreateDefaultSubobject<UPlantStageComponent>(TEXT("Stage"));VisualComponent=CreateDefaultSubobject<UPlantVisualComponent>(TEXT("Visual"));SoilComponent=CreateDefaultSubobject<USoilCellComponent>(TEXT("Soil"));HarvestComponent=CreateDefaultSubobject<UHarvestComponent>(TEXT("Harvest"));CreateDefaultSubobject<USoilMoistureComponent>(TEXT("SoilMoisture"));CreateDefaultSubobject<USoilNutrientComponent>(TEXT("SoilNutrients"));CreateDefaultSubobject<USoilPHComponent>(TEXT("SoilPH"));}
void ACropFieldActor::BeginPlay(){Super::BeginPlay();if(USimulationSubsystem* Sim=GetWorld()->GetSubsystem<USimulationSubsystem>())Sim->OnDayTick.AddDynamic(this,&ACropFieldActor::HandleSimulationDay);if(StageComponent)StageComponent->OnStageChanged.AddDynamic(this,&ACropFieldActor::HandleStageChanged);}
void ACropFieldActor::PlantCrop(UPlantDefinition* Def,int32 Count)
{
 if(!Def||!GrowthComponent||Count<=0)return;
 FString Error;if(!Def->ValidateDefinition(Error)){UE_LOG(LogTemp,Warning,TEXT("Crop planting rejected: %s"),*Error);return;}
 GrowthComponent->SetDefinition(Def);GrowthComponent->PlantState.SeedCount=Count;
 if(VisualComponent)VisualComponent->SetDefinition(Def);
 if(StageComponent)StageComponent->CheckStageTransition(EGrowthStage::Seed);
 if(UWorldStateSubsystem* WS=GetWorld()?GetWorld()->GetSubsystem<UWorldStateSubsystem>():nullptr)
   if(UChunkState* Chunk=WS->GetOrCreateChunkAtWorld(GetActorLocation()))
      if(Chunk->Metadata.ActivationMode==EChunkActivationMode::Passive)Chunk->Metadata.ActivationMode=EChunkActivationMode::Scheduled;
}
bool ACropFieldActor::PlantCropFromInventory(UPlantDefinition* Def,UInventoryComponent* Inventory,FName SeedItemID,int32 SeedCount)
{
 if(!Def||!Inventory||SeedItemID.IsNone()||SeedCount<=0||!GrowthComponent||!Inventory->HasItem(SeedItemID,SeedCount))return false;
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
 return ActualYield;
}
void ACropFieldActor::HandleSimulationDay(int32){UpdateDaily();}
void ACropFieldActor::UpdateDaily(){if(!GetWorld()||!GrowthComponent||!SoilComponent)return;USimulationSubsystem* Sim=GetWorld()->GetSubsystem<USimulationSubsystem>();USimulationState* S=Sim?Sim->GetState():nullptr;if(!S)return;
 UWorldStateSubsystem* WorldState=GetWorld()->GetSubsystem<UWorldStateSubsystem>();
 FChunkCell* SharedCell=WorldState?WorldState->GetCellAtWorld(GetActorLocation()):nullptr;
 if(SharedCell)
 {
   FSoilState& Soil=SoilComponent->GetMutableSoilState();
   Soil.Water=FMath::Clamp(SharedCell->SoilMoisture,0.0f,1.0f)*FMath::Max(1.0f,Soil.WaterMax);
   Soil.Nitrogen=SharedCell->SoilNitrogen;Soil.Phosphorus=SharedCell->SoilPhosphorus;Soil.Potassium=SharedCell->SoilPotassium;Soil.pH=SharedCell->SoilPH;
   if(UChunkState* Chunk=WorldState->GetOrCreateChunkAtWorld(GetActorLocation()))
      if(Chunk->Metadata.ActivationMode==EChunkActivationMode::Passive)Chunk->Metadata.ActivationMode=EChunkActivationMode::Scheduled;
 }
 float Ks=1.f,Kn=1.f,PH=1.f; if(auto* M=FindComponentByClass<USoilMoistureComponent>())Ks=M->ComputeWaterStress(SoilComponent->GetSoilState().Water,SoilComponent->GetSoilState().WaterWilting,SoilComponent->GetSoilState().WaterCritical);if(auto* N=FindComponentByClass<USoilNutrientComponent>()){const auto& Soil=SoilComponent->GetSoilState();Kn=FMath::Min3(N->ComputeSuitability(Soil.Nitrogen,GrowthComponent->Definition?GrowthComponent->Definition->NitrogenDemand:100.f),N->ComputeSuitability(Soil.Phosphorus,GrowthComponent->Definition?GrowthComponent->Definition->PhosphorusDemand:50.f),N->ComputeSuitability(Soil.Potassium,GrowthComponent->Definition?GrowthComponent->Definition->PotassiumDemand:80.f));}if(auto* P=FindComponentByClass<USoilPHComponent>();P&&GrowthComponent->Definition)PH=P->ComputePHSuitability(GrowthComponent->Definition->OptimalPH,GrowthComponent->Definition->PHTolerance);GrowthComponent->SetStress(Ks,Kn*PH,SoilComponent->GetSoilState().WeedBiomass/500.f);
 if(!SharedCell){SoilComponent->UpdateDaily(S->Rainfall,0.f,2.f,2.f,0.f,1.f);if(auto* N=FindComponentByClass<USoilNutrientComponent>())N->ApplyDailyLeaching();}
 const float BiomassBefore=GrowthComponent->GetState().Biomass;
 GrowthComponent->UpdateDaily(S->TempMax,S->TempMin,1.f);
 if(SharedCell)
 {
   const FPlantState& Plant=GrowthComponent->GetState();
   const UPlantDefinition* Def=GrowthComponent->GetDefinition();
   const float BiomassGain=FMath::Max(0.0f,Plant.Biomass-BiomassBefore);
   const FSoilState& Soil=SoilComponent->GetSoilState();
   SharedCell->SoilMoisture=FMath::Clamp(SharedCell->SoilMoisture-(5.0f*Ks/FMath::Max(1.0f,Soil.WaterMax)),0.0f,1.0f);
   if(Def&&Def->MaxBiomass>KINDA_SMALL_NUMBER)
   {
     const float GrowthRatio=FMath::Clamp(BiomassGain/Def->MaxBiomass,0.0f,1.0f);
     SharedCell->SoilNitrogen=FMath::Max(0.0f,SharedCell->SoilNitrogen-GrowthRatio*FMath::Max(0.0f,Def->NitrogenDemand));
     SharedCell->SoilPhosphorus=FMath::Max(0.0f,SharedCell->SoilPhosphorus-GrowthRatio*FMath::Max(0.0f,Def->PhosphorusDemand));
     SharedCell->SoilPotassium=FMath::Max(0.0f,SharedCell->SoilPotassium-GrowthRatio*FMath::Max(0.0f,Def->PotassiumDemand));
   }
   FSoilState& MutableSoil=SoilComponent->GetMutableSoilState();
   MutableSoil.Water=SharedCell->SoilMoisture*FMath::Max(1.0f,MutableSoil.WaterMax);
   MutableSoil.Nitrogen=SharedCell->SoilNitrogen;MutableSoil.Phosphorus=SharedCell->SoilPhosphorus;MutableSoil.Potassium=SharedCell->SoilPotassium;MutableSoil.pH=SharedCell->SoilPH;
   SharedCell->bIsDirty=true;
   if(WorldState)if(UChunkState* Chunk=WorldState->GetOrCreateChunkAtWorld(GetActorLocation()))Chunk->MarkDirty();
 }if(StageComponent)StageComponent->CheckStageTransition(GrowthComponent->GetCurrentStage());if(VisualComponent)VisualComponent->UpdateVisual(GrowthComponent->GetCurrentStage(),GrowthComponent->GetState().StageProgress);}
void ACropFieldActor::HandleStageChanged(EGrowthStage,EGrowthStage NewStage){if(VisualComponent&&GrowthComponent)VisualComponent->UpdateVisual(NewStage,GrowthComponent->GetState().StageProgress);}
