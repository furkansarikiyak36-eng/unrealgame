#include "RaidTestHarness.h"
#include "RaidPhaseR2_Difficulty/RaidDifficultyScorer.h"
#include "RaidPhaseR2_Difficulty/RaidStageSelector.h"
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
bool URaidTestHarness::RunAllTests(FString& Out){Out.Empty();int32 Pass=0,Total=0;const TPair<const TCHAR*,bool(URaidTestHarness::*)()> Tests[]={{TEXT("DifficultyScoring"),&URaidTestHarness::TestDifficultyScoring},{TEXT("PartyScore"),&URaidTestHarness::TestPartyScore},{TEXT("StageSelection"),&URaidTestHarness::TestStageSelection},{TEXT("WeightedPicker"),&URaidTestHarness::TestWeightedPicker},{TEXT("AStarPathfinding"),&URaidTestHarness::TestAStarPathfinding},{TEXT("BudgetMath"),&URaidTestHarness::TestBudgetMath},{TEXT("SpawnDecision"),&URaidTestHarness::TestSpawnDecision}};for(const auto&T:Tests){++Total;const bool B=(this->*T.Value)();if(B)++Pass;Out+=FString::Printf(TEXT("%s: %s\n"),T.Key,B?TEXT("PASS"):TEXT("FAIL"));}Out+=FString::Printf(TEXT("\nTotal: %d/%d\n"),Pass,Total);return Pass==Total;}
