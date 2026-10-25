#include "RaidTestHarness.h"
#include "RaidPhaseR2_Difficulty/RaidDifficultyScorer.h"
#include "RaidPhaseR2_Difficulty/RaidStageSelector.h"
#include "RaidPhaseR2_Difficulty/RaidStageTableAsset.h"
#include "RaidPhaseR3_Planner/HordePlanAsset.h"
#include "RaidPhaseR1_Trigger/RaidLifecycleComponent.h"
#include "RaidPhaseR8_Defense/StructureDamageResolver.h"
#include <limits>
#include "RaidPhaseR3_Planner/HordeCompositionPicker.h"
#include "RaidPhaseR4_Budget/SpawnBudgetTracker.h"
#include "RaidPhaseR4_Budget/SpawnCapacityCalculator.h"
#include "RaidPhaseR7_Navigation/NavGraphBuilder.h"
#include "RaidPhaseR7_Navigation/AStarPathfinder.h"
#include "Algo/Reverse.h"
bool URaidTestHarness::TestDifficultyScoring(){return URaidDifficultyScorer::ComputeSingleScore(10,.5f,0.f,1.f)==10;}
bool URaidTestHarness::TestPartyScore(){return URaidDifficultyScorer::ComputePartyScore({100,80,60,40},1.f,.8f)==222;}
bool URaidTestHarness::TestStageSelection(){return URaidStageSelector::BinarySearchThreshold(100,{10,30,50,70,90,103,120})==90;}
bool URaidTestHarness::TestWeightedPicker(){TArray<FWeightedEntry>P;FWeightedEntry A;A.EnemyID=TEXT("A");A.Weight=1;P.Add(A);FWeightedEntry B;B.EnemyID=TEXT("B");B.Weight=1;P.Add(B);for(int32 I=0;I<20;++I){const FName N=UHordeCompositionPicker::PickWeightedEnemy(P);if(N!=TEXT("A")&&N!=TEXT("B"))return false;}return true;}
bool URaidTestHarness::TestAStarPathfinding(){UNavGraph*G=NewObject<UNavGraph>(this);G->AddNode(FVector::ZeroVector);G->AddNode(FVector(100,0,0));G->ConnectNodes(0,1);UAStarPathfinder*P=NewObject<UAStarPathfinder>(this);return P->FindPath(G,FVector::ZeroVector,FVector(100,0,0)).Num()==2;}
bool URaidTestHarness::TestBudgetMath(){USpawnBudgetTracker*T=NewObject<USpawnBudgetTracker>(this);T->Initialize(40);T->OnSpawned(27);return T->GetRemainingBudget()==13;}
bool URaidTestHarness::TestSpawnDecision(){USpawnCapacityCalculator*C=NewObject<USpawnCapacityCalculator>(this);C->WaveCapacity=8;C->GlobalCapacity=20;C->EventCapacity=15;return C->ComputeEffectiveCapacity()==8;}
bool URaidTestHarness::TestRaidPlanValidation()
{
    UHordePlanAsset* Plan=NewObject<UHordePlanAsset>(this);
    Plan->PlanID=TEXT("ContractPlan");
    FWavePlan Wave;Wave.GroupID=TEXT("Raiders");Wave.TotalCount=4;Wave.MaxAlive=2;Wave.Duration=30.f;Wave.Intermission=2.f;
    Plan->Waves.Add(Wave);
    FString Error;
    if(!Plan->ValidatePlan(Error))return false;
    Plan->Waves[0].Duration=86401.f;
    return !Plan->ValidatePlan(Error);
}
bool URaidTestHarness::TestRaidStageBudgetValidation()
{
    URaidStageTableAsset* Table=NewObject<URaidStageTableAsset>(this);
    FRaidStageConfig Stage;Stage.StageIndex=0;Stage.Threshold=0;Stage.Budget.ConcurrentPopulation=4;Stage.Budget.TotalBudget=8;Stage.Budget.RaidDuration=120.f;
    Table->Stages.Add(Stage);
    FString Error;
    if(!Table->ValidateTable(Error))return false;
    Table->Stages[0].Budget.TotalBudget=100001;
    return !Table->ValidateTable(Error);
}
bool URaidTestHarness::TestRaidLifecycleTransitions()
{
    URaidLifecycleComponent* Lifecycle=NewObject<URaidLifecycleComponent>(this);
    FRaidContext Context;
    Lifecycle->BeginEvent(Context);
    if(!Context.bEventActive||Context.Lifecycle!=ERaidLifecycleState::Preparing)return false;
    Lifecycle->SetPhase(Context,ERaidLifecycleState::Active);
    if(Context.Lifecycle!=ERaidLifecycleState::Active)return false;
    Lifecycle->SetPhase(Context,ERaidLifecycleState::Intermission);
    if(Context.Lifecycle!=ERaidLifecycleState::Intermission)return false;
    Lifecycle->EndEvent(Context);
    return !Context.bEventActive&&Context.Lifecycle==ERaidLifecycleState::Completed;
}
bool URaidTestHarness::TestStructureDamageSanitization()
{
    UStructureDamageResolver* Resolver=NewObject<UStructureDamageResolver>(this);
    const float Invalid=std::numeric_limits<float>::quiet_NaN();
    return Resolver->ComputeDamage(Invalid,0.f,1.f)==0.f
        &&Resolver->ComputeDamage(10.f,Invalid,1.f)==0.f
        &&Resolver->ComputeDamage(10.f,0.f,Invalid)==0.f;
}
bool URaidTestHarness::RunAllTests(FString& Out){Out.Empty();int32 Pass=0,Total=0;const TPair<const TCHAR*,bool(URaidTestHarness::*)()> Tests[]={{TEXT("DifficultyScoring"),&URaidTestHarness::TestDifficultyScoring},{TEXT("PartyScore"),&URaidTestHarness::TestPartyScore},{TEXT("StageSelection"),&URaidTestHarness::TestStageSelection},{TEXT("WeightedPicker"),&URaidTestHarness::TestWeightedPicker},{TEXT("AStarPathfinding"),&URaidTestHarness::TestAStarPathfinding},{TEXT("BudgetMath"),&URaidTestHarness::TestBudgetMath},{TEXT("SpawnDecision"),&URaidTestHarness::TestSpawnDecision},{TEXT("RaidPlanValidation"),&URaidTestHarness::TestRaidPlanValidation},{TEXT("RaidStageBudgetValidation"),&URaidTestHarness::TestRaidStageBudgetValidation},{TEXT("RaidLifecycleTransitions"),&URaidTestHarness::TestRaidLifecycleTransitions},{TEXT("StructureDamageSanitization"),&URaidTestHarness::TestStructureDamageSanitization}};for(const auto&T:Tests){++Total;const bool B=(this->*T.Value)();if(B)++Pass;Out+=FString::Printf(TEXT("%s: %s\n"),T.Key,B?TEXT("PASS"):TEXT("FAIL"));}Out+=FString::Printf(TEXT("\nTotal: %d/%d\n"),Pass,Total);return Pass==Total;}
