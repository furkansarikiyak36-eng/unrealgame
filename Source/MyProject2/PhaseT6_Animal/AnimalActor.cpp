#include "AnimalActor.h"
#include "Engine/World.h"
#include "AnimalGrowthComponent.h"
#include "AnimalNutritionComponent.h"
#include "AnimalEnergyComponent.h"
#include "AnimalHealthComponent.h"
#include "AnimalDefinition.h"
#include "AnimalReproductionComponent.h"
#include "PregnancyComponent.h"
#include "AnimalProductionComponent.h"
#include "PhaseT7_Production/MilkProductionComponent.h"
#include "PhaseT7_Production/EggProductionComponent.h"
#include "PhaseT7_Production/WoolProductionComponent.h"
#include "FeedStorageComponent.h"
#include "PhaseT9_Integration/ManureComponent.h"
#include "InventoryCore/InventoryComponent.h"
#include "WorldCore/WorldStateSubsystem.h"
#include "WorldCore/ChunkState.h"
#include "SimCore/SimulationSubsystem.h"
#include "PhaseT8_Reproduction/HerdManager.h"
#include "Components/StaticMeshComponent.h"
AAnimalActor::AAnimalActor(){PrimaryActorTick.bCanEverTick=false;AnimalMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));RootComponent=AnimalMesh;GrowthComponent=CreateDefaultSubobject<UAnimalGrowthComponent>(TEXT("Growth"));NutritionComponent=CreateDefaultSubobject<UAnimalNutritionComponent>(TEXT("Nutrition"));EnergyComponent=CreateDefaultSubobject<UAnimalEnergyComponent>(TEXT("Energy"));HealthComponent=CreateDefaultSubobject<UAnimalHealthComponent>(TEXT("Health"));ReproductionComponent=CreateDefaultSubobject<UAnimalReproductionComponent>(TEXT("Reproduction"));ProductionComponent=CreateDefaultSubobject<UAnimalProductionComponent>(TEXT("Production"));PregnancyComponent=CreateDefaultSubobject<UPregnancyComponent>(TEXT("Pregnancy"));MilkProduction=CreateDefaultSubobject<UMilkProductionComponent>(TEXT("Milk"));EggProduction=CreateDefaultSubobject<UEggProductionComponent>(TEXT("Eggs"));WoolProduction=CreateDefaultSubobject<UWoolProductionComponent>(TEXT("Wool"));FeedStorage=CreateDefaultSubobject<UFeedStorageComponent>(TEXT("FeedStorage"));ManureComponent=CreateDefaultSubobject<UManureComponent>(TEXT("Manure"));}
void AAnimalActor::BeginPlay(){Super::BeginPlay();if(USimulationSubsystem* S=GetWorld()->GetSubsystem<USimulationSubsystem>())S->OnDayTick.AddDynamic(this,&AAnimalActor::HandleSimulationDay);if(UHerdManager* H=GetWorld()->GetSubsystem<UHerdManager>())H->RegisterAnimal(this);if(PregnancyComponent)PregnancyComponent->OnBirth.AddDynamic(this,&AAnimalActor::HandleBirth);}
void AAnimalActor::EndPlay(const EEndPlayReason::Type R){if(GetWorld())if(UHerdManager* H=GetWorld()->GetSubsystem<UHerdManager>())H->UnregisterAnimal(this);if(GetWorld())if(USimulationSubsystem* S=GetWorld()->GetSubsystem<USimulationSubsystem>())S->OnDayTick.RemoveDynamic(this,&AAnimalActor::HandleSimulationDay);Super::EndPlay(R);}
void AAnimalActor::HandleSimulationDay(int32){UpdateDaily();}
void AAnimalActor::HandleBirth(int32){if(Definition&&(Definition->Species==EAnimalSpecies::Cow||Definition->Species==EAnimalSpecies::Goat))State.LactationDays=1.f;State.bIsPregnant=false;State.PregnancyDays=0.f;}
void AAnimalActor::Initialize(UAnimalDefinition* Def,EAnimalSex Sex,float Age){if(!Def)return;Definition=Def;State.Sex=Sex;State.AgeDays=FMath::Max(0.f,Age);State.BodyWeight=Def->GompertzWeight(Age);if(GrowthComponent){GrowthComponent->Definition=Def;GrowthComponent->InitializeAge(Age);}if(NutritionComponent)NutritionComponent->Definition=Def;if(EnergyComponent)EnergyComponent->Definition=Def;if(ReproductionComponent)ReproductionComponent->Definition=Def;if(PregnancyComponent)PregnancyComponent->Definition=Def;if(MilkProduction)MilkProduction->Definition=Def;if(EggProduction)EggProduction->Definition=Def;if(WoolProduction)WoolProduction->Definition=Def;if(ProductionComponent)ProductionComponent->Definition=Def;}
void AAnimalActor::UpdateDaily(){if(!Definition||!State.bIsAlive)return;const float HealthFactor=HealthComponent?HealthComponent->GetHealth()/100.f:1.f;const float Imax=NutritionComponent?NutritionComponent->ComputeIntakeCapacity(State.BodyWeight,1.f,HealthFactor):0.f;float Intake=Imax;FFeedItem Feed;bool bHasFeed=false;float FeedAmount=0.f;FName SelectedFeed=NAME_None;float AvailableFeed=0.f;if(FeedStorage){for(const auto& Pair:FeedStorage->FeedStock){if(Pair.Value>0.f&&FeedStorage->GetFeedDefinition(Pair.Key,Feed)){SelectedFeed=Pair.Key;AvailableFeed=Pair.Value;break;}}if(!SelectedFeed.IsNone()){FeedAmount=FMath::Min(AvailableFeed,Imax);bHasFeed=FeedStorage->ConsumeFeed(SelectedFeed,FeedAmount);}}if(bHasFeed&&NutritionComponent)Intake=NutritionComponent->ComputeEnergyIntake(FeedAmount,Feed);else Intake=0.f;if(ManureComponent&&bHasFeed&&NutritionComponent){const float ProteinIntake=NutritionComponent->ComputeProteinIntake(FeedAmount,Feed);ManureComponent->AccumulateManure(FeedAmount*0.25f,ProteinIntake*0.65f);}if(GrowthComponent){GrowthComponent->UpdateDaily(1.f,bHasFeed?NutritionComponent->ComputeSuitability(FeedAmount,FMath::Max(.01f,Imax)):1.f,HealthFactor,1.f);State.BodyWeight=GrowthComponent->GetWeight();State.AgeDays=GrowthComponent->GetAge();}if(EnergyComponent){const bool bLactating=State.LactationDays>0.f;const float Req=EnergyComponent->ComputeRequirements(State.BodyWeight,State.AgeDays<Definition->AgeMaturityDays,bLactating,State.bIsPregnant,0.f);EnergyComponent->UpdateEnergyBalance(Intake,Req,1.f);State.BCS=EnergyComponent->GetBCS();State.EnergyBalance=EnergyComponent->GetEnergyBalance();}if(HealthComponent){HealthComponent->UpdateHealth(.5f,State.BCS<2.f?1.f:0.f,0.f,1.f);State.Health=HealthComponent->GetHealth();if(State.Health<=0.f)State.bIsAlive=false;}if(PregnancyComponent&&PregnancyComponent->IsPregnant()){int32 FreeCapacity=0;if(GetWorld())if(UHerdManager* H=GetWorld()->GetSubsystem<UHerdManager>())FreeCapacity=H->GetFreeCapacity();PregnancyComponent->UpdatePregnancy(1.f,FreeCapacity);State.bIsPregnant=PregnancyComponent->IsPregnant();}if(ReproductionComponent&&State.Sex==EAnimalSex::Female)ReproductionComponent->UpdateReproduction(State.AgeDays,State.Health,State.BCS,1.f,1.f,SimulationSeed);if(Definition->Species==EAnimalSpecies::Cow||Definition->Species==EAnimalSpecies::Goat){if(State.LactationDays>0.f&&MilkProduction){if(State.LactationDays>Definition->LactationLength)State.LactationDays=0.f;else{MilkProduction->ComputeActualMilk(MilkProduction->ComputePotentialMilk(State.LactationDays),1.f,1.f,HealthFactor,1.f);++State.LactationDays;}}}if(Definition->Species==EAnimalSpecies::Chicken&&EggProduction)EggProduction->ComputeDailyEggs(State.AgeDays>=Definition->AgeMaturityDays?1.f:0.f,1.f,HealthFactor,1.f);if(Definition->Species==EAnimalSpecies::Sheep&&WoolProduction)WoolProduction->AccumulateDaily(1.f,HealthFactor,1.f);State.PhysioState=State.Health<50.f?EAnimalPhysiologicalState::Sick:(State.bIsPregnant?EAnimalPhysiologicalState::Pregnant:(State.LactationDays>0.f?EAnimalPhysiologicalState::Lactating:(State.AgeDays>=Definition->AgeMaturityDays?EAnimalPhysiologicalState::Adult:EAnimalPhysiologicalState::Growing)));State.PregnancyDays=State.bIsPregnant?FMath::Min(Definition->GestationDays,State.PregnancyDays+1.f):0.f;}

int32 AAnimalActor::CollectProductsToInventory(UInventoryComponent* Destination,FName MilkItemID,FName EggItemID,FName WoolItemID)
{
 if(!IsValid(Destination))return 0;
 int32 CollectedUnits=0;

 if(MilkProduction&&!MilkItemID.IsNone()&&FMath::IsFinite(MilkProduction->StoredMilk))
 {
   const int32 Units=FMath::Max(0,FMath::FloorToInt(MilkProduction->StoredMilk));
   if(Units>0&&Destination->CanAddItem(MilkItemID,Units)&&Destination->AddItem(MilkItemID,Units)==EInventoryResult::Success)
   {
     const float Removed=MilkProduction->Milking(static_cast<float>(Units));
     if(FMath::IsNearlyEqual(Removed,static_cast<float>(Units)))CollectedUnits+=Units;
     else Destination->RemoveItem(MilkItemID,Units);
   }
 }

 if(EggProduction&&!EggItemID.IsNone()&&FMath::IsFinite(EggProduction->EggAccumulator))
 {
   const int32 Units=FMath::Max(0,FMath::FloorToInt(EggProduction->EggAccumulator));
   if(Units>0&&Destination->CanAddItem(EggItemID,Units)&&Destination->AddItem(EggItemID,Units)==EInventoryResult::Success)
   {
     const int32 Removed=EggProduction->CollectEggs();
     if(Removed==Units)CollectedUnits+=Units;
     else Destination->RemoveItem(EggItemID,Units);
   }
 }

 if(WoolProduction&&!WoolItemID.IsNone()&&FMath::IsFinite(WoolProduction->AccumulatedWool)&&
    WoolProduction->DaysSinceShearing>=WoolProduction->DaysBetweenShearing)
 {
   const int32 Units=FMath::Max(0,FMath::FloorToInt(WoolProduction->AccumulatedWool));
   if(Units>0&&Destination->CanAddItem(WoolItemID,Units)&&Destination->AddItem(WoolItemID,Units)==EInventoryResult::Success)
   {
     WoolProduction->AccumulatedWool=FMath::Max(0.0f,WoolProduction->AccumulatedWool-static_cast<float>(Units));
     WoolProduction->DaysSinceShearing=0.0f;
     CollectedUnits+=Units;
   }
 }
 return CollectedUnits;
}

bool AAnimalActor::ApplyManureToSoil(const FVector& SoilWorldLocation,float Amount)
{
 if(!ManureComponent||!GetWorld()||!FMath::IsFinite(Amount)||Amount<=0.0f||
    !FMath::IsFinite(ManureComponent->ManureAmount)||ManureComponent->ManureAmount<=0.0f)return false;
 UWorldStateSubsystem* WorldState=GetWorld()->GetSubsystem<UWorldStateSubsystem>();
 if(!WorldState)return false;
 FChunkCell* Cell=WorldState->GetCellAtWorld(SoilWorldLocation);
 if(!Cell||ManureComponent->ManureAmount<=0.0f)return false;

 const float AvailableManure=ManureComponent->ManureAmount;
 const float NitrogenPerUnit=AvailableManure>KINDA_SMALL_NUMBER
   ?FMath::Max(0.0f,ManureComponent->ManureNitrogen)/AvailableManure:0.0f;
 const float Collected=ManureComponent->CollectManure(Amount);
 if(!FMath::IsFinite(Collected)||Collected<=KINDA_SMALL_NUMBER)return false;
 const float AppliedNitrogen=Collected*NitrogenPerUnit;
 // Manure is debited only after the target canonical world cell has been acquired.
 Cell->SoilNitrogen=FMath::Clamp(Cell->SoilNitrogen+AppliedNitrogen,0.0f,1.0e9f);
 Cell->SoilPhosphorus=FMath::Clamp(Cell->SoilPhosphorus+AppliedNitrogen*0.35f,0.0f,1.0e9f);
 Cell->SoilPotassium=FMath::Clamp(Cell->SoilPotassium+AppliedNitrogen*0.5f,0.0f,1.0e9f);
 Cell->bIsDirty=true;
 if(UChunkState* Chunk=WorldState->GetOrCreateChunkAtWorld(SoilWorldLocation))
 {
   if(Chunk->Metadata.ActivationMode==EChunkActivationMode::Passive)Chunk->Metadata.ActivationMode=EChunkActivationMode::Scheduled;
   Chunk->MarkDirty();
 }
 return true;
}
