#include "RaidDirectorSubsystem.h"
#include "RaidFeedbackBus.h"
#include "RaidPhaseR1_Trigger/RaidSchedulerComponent.h"
#include "RaidPhaseR1_Trigger/RaidDebouncer.h"
#include "RaidPhaseR1_Trigger/RaidLifecycleComponent.h"
#include "RaidPhaseR2_Difficulty/RaidStageTableAsset.h"
#include "RaidPhaseR2_Difficulty/RaidDifficultyScorer.h"
#include "RaidPhaseR2_Difficulty/RaidStageSelector.h"
#include "RaidPhaseR3_Planner/HordePlanAsset.h"
#include "RaidPhaseR3_Planner/HordePlanSelector.h"
#include "RaidPhaseR3_Planner/HordeGroupResolver.h"
#include "RaidPhaseR3_Planner/HordeCompositionPicker.h"
#include "RaidPhaseR4_Budget/SpawnBudgetTracker.h"
#include "RaidPhaseR4_Budget/SpawnCapacityCalculator.h"
#include "RaidPhaseR4_Budget/SpawnRequestLimiter.h"
#include "RaidPhaseR4_Budget/PopulationCounter.h"
#include "RaidPhaseR5_Spawn/SpawnPositionFinder.h"
#include "RaidPhaseR5_Spawn/SpawnLoadBalancer.h"
#include "RaidPhaseR9_WaveState/RaidWaveStateMachine.h"
#include "RaidPhaseR10_Crosscutting/RaidFrameBudget.h"
#include "RaidPhaseR10_Crosscutting/RaidTimeBudget.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "RaidPhaseR3_Planner/HordePlanAsset.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

void URaidDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection){Super::Initialize(Collection);Scheduler=NewObject<URaidSchedulerComponent>(this);Debouncer=NewObject<URaidDebouncer>(this);Lifecycle=NewObject<URaidLifecycleComponent>(this);Budget=NewObject<USpawnBudgetTracker>(this);Capacity=NewObject<USpawnCapacityCalculator>(this);Limiter=NewObject<USpawnRequestLimiter>(this);Finder=NewObject<USpawnPositionFinder>(this);Balancer=NewObject<USpawnLoadBalancer>(this);Counter=NewObject<UPopulationCounter>(this);WaveStateMachine=NewObject<URaidWaveStateMachine>(this);FrameBudget=NewObject<URaidFrameBudget>(this);TimeBudget=NewObject<URaidTimeBudget>(this);FeedbackBus=NewObject<URaidFeedbackBus>(this);}
void URaidDirectorSubsystem::Deinitialize(){if(Context.bEventActive)CancelRaid(TEXT("WorldSubsystemDeinitialize"));Super::Deinitialize();}
TStatId URaidDirectorSubsystem::GetStatId()const{RETURN_QUICK_DECLARE_CYCLE_STAT(URaidDirectorSubsystem,STATGROUP_Tickables);}
void URaidDirectorSubsystem::ConfigureRaidAssets(URaidStageTableAsset*T,const TArray<UHordePlanAsset*>&P,UEnemyGroupLibrary*G,UEnemyLibrary*E){StageTable=T;AvailablePlans.Reset();for(UHordePlanAsset*Plan:P)if(Plan)AvailablePlans.Add(Plan);EnemyGroups=G;Enemies=E;}
bool URaidDirectorSubsystem::SetAutomaticSchedulingEnabled(bool bEnabled,float IntervalSeconds,float JitterMinSeconds,float JitterMaxSeconds,ERaidEventType EventType)
{
    if(!FMath::IsFinite(IntervalSeconds)||!FMath::IsFinite(JitterMinSeconds)||!FMath::IsFinite(JitterMaxSeconds)
        ||IntervalSeconds<30.0f||IntervalSeconds>604800.0f||EventType==ERaidEventType::None)
    {
        return false;
    }
    bAutomaticSchedulingEnabled=bEnabled;
    AutoScheduleIntervalSeconds=IntervalSeconds;
    AutoScheduleJitterMinSeconds=FMath::Clamp(JitterMinSeconds,-86400.0f,86400.0f);
    AutoScheduleJitterMaxSeconds=FMath::Clamp(JitterMaxSeconds,-86400.0f,86400.0f);
    AutoScheduleEventType=EventType;
    if(!bEnabled)
    {
        Scheduler->ResetSchedule();
        return true;
    }
    if(!GetWorld()||!Scheduler)return false;
    Scheduler->ScheduleNextEvent(GetWorld()->GetTimeSeconds()+AutoScheduleIntervalSeconds,AutoScheduleJitterMinSeconds,AutoScheduleJitterMaxSeconds);
    return true;
}
void URaidDirectorSubsystem::Tick(float Dt)
{
    Super::Tick(Dt);
    UWorld* World=GetWorld();
    if(!World)return;
    if(!Context.bEventActive)
    {
        if(bAutomaticSchedulingEnabled&&Scheduler&&Scheduler->IsEventDue(World->GetTimeSeconds()))
        {
            StartRaid(AutoScheduleEventID,AutoScheduleEventType);
            if(!Context.bEventActive)Scheduler->ScheduleNextEvent(World->GetTimeSeconds()+60.0f,0.0f,0.0f);
        }
        return;
    }
    TimeBudget->BeginTick();FrameBudget->ResetFrame();
    ElapsedEventTime+=FMath::Clamp(FMath::IsFinite(Dt)?Dt:0.f,0.f,5.f);
    ProcessWaveTick(Dt);
    if(TimeBudget->HasTimeRemaining())ProcessSpawns(Dt);
}
void URaidDirectorSubsystem::StartRaid(FName ID,ERaidEventType Type)
{
    if(!GetWorld()||ID.IsNone()||Type==ERaidEventType::None||Context.bEventActive||!Debouncer->TryAcquireEventLock(ID,60.f))return;
    Context.Reset();Context.EventID=ID;Context.EventType=Type;Context.bEventActive=true;
    Context.EventStartTime=GetWorld()->GetTimeSeconds();ElapsedEventTime=0.f;BudgetWaveIndex=INDEX_NONE;CurrentPlan=nullptr;
    CaptureContext();
    FString ValidationError;
    if(Context.Players.IsEmpty()||(StageTable&&!StageTable->ValidateTable(ValidationError)))
    {
        Context.bEventActive=false;Context.Lifecycle=ERaidLifecycleState::Cancelled;Debouncer->ReleaseEventLock(ID);return;
    }
    EvaluateDifficulty();BuildPlan();
    if(!ValidateCurrentPlan(ValidationError))
    {
        UE_LOG(LogTemp,Warning,TEXT("Raid '%s' rejected before activation: %s"),*ID.ToString(),*ValidationError);
        Context.bEventActive=false;Context.Lifecycle=ERaidLifecycleState::Cancelled;CurrentPlan=nullptr;Debouncer->ReleaseEventLock(ID);return;
    }
    WaveStateMachine->Initialize(CurrentPlan->Waves.Num());BudgetWaveIndex=0;
    Budget->Initialize(CurrentPlan->Waves[0].TotalCount);Capacity->WaveCapacity=CurrentPlan->Waves[0].MaxAlive;
    Lifecycle->BeginEvent(Context);OnEventStarted.Broadcast(ID);
    if(bAutomaticSchedulingEnabled&&Scheduler)Scheduler->ScheduleNextEvent(GetWorld()->GetTimeSeconds()+AutoScheduleIntervalSeconds,AutoScheduleJitterMinSeconds,AutoScheduleJitterMaxSeconds);
    FRaidFeedbackEvent Event;Event.Type=Type;Event.SourceModule=TEXT("Trigger");Event.TargetModule=TEXT("Difficulty");
    Event.NumericData.Add(TEXT("DifficultyScore"),(float)Context.DifficultyScore);FeedbackBus->Publish(TEXT("RaidStarted"),Event);
}
void URaidDirectorSubsystem::EndRaid()
{
    if(!Context.bEventActive)return;
    const FName ID=Context.EventID;
    Context.bEventActive=false;
    Lifecycle->EndEvent(Context);
    Debouncer->ReleaseEventLock(ID);
    // Survivors may finish their AI behavior after the event; they are no longer
    // cancellable as members of a future event, but still report removal to Counter.
    ManagedEnemies.Reset();
    OnEventEnded.Broadcast(ID);
}
void URaidDirectorSubsystem::CancelRaid(FName Reason)
{
    if(!Context.bEventActive)return;
    const FName ID=Context.EventID;
    Context.bEventActive=false;
    Lifecycle->CancelEvent(Context,Reason);
    CleanupManagedEnemies();
    Debouncer->ReleaseEventLock(ID);
    OnEventEnded.Broadcast(ID);
}
void URaidDirectorSubsystem::CaptureContext(){Context.Players.Empty();if(!GetWorld())return;for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It){APlayerController*PC=It->Get();if(!PC)continue;APawn*Pawn=PC->GetPawn();if(!Pawn)continue;FPlayerStateSnapshot S;S.PlayerId=PC->GetUniqueID();S.Location=Pawn->GetActorLocation();S.bIsAlive=true;if(APlayerState*PS=PC->GetPlayerState<APlayerState>())S.Level=1;Context.Players.Add(S);}Context.CapturedTime=GetWorld()->GetTimeSeconds();Context.World.InGameTime=Context.CapturedTime;Context.World.WorldMultiplier=1.f;}
void URaidDirectorSubsystem::EvaluateDifficulty(){if(!StageTable){Context.DifficultyStage=0;Context.DifficultyScore=0;return;}TArray<int32> Scores;for(const FPlayerStateSnapshot&P:Context.Players)Scores.Add(URaidDifficultyScorer::ComputeSingleScore(P.Level,P.SurvivalTime*StageTable->SurvivalTimeMultiplier,P.DeathCount*StageTable->DeathPenaltyFactor,StageTable->LevelMultiplier));Scores.Sort([](int32 A,int32 B){return A>B;});Context.DifficultyScore=URaidDifficultyScorer::ComputePartyScore(Scores,StageTable->PartyWeightFirst,StageTable->PartyWeightDecay);Context.DifficultyStage=URaidStageSelector::FindNearestStage(Context.DifficultyScore,StageTable);Context.bScoreFrozen=true;}
void URaidDirectorSubsystem::BuildPlan(){TArray<UHordePlanAsset*> PlanPointers;for(UHordePlanAsset* Plan:AvailablePlans)if(Plan)PlanPointers.Add(Plan);CurrentPlan=UHordePlanSelector::SelectPlanForStage(Context.DifficultyStage,PlanPointers);Context.bPlanFrozen=CurrentPlan!=nullptr;if(!CurrentPlan)return;if(StageTable)if(const FRaidStageConfig*C=StageTable->FindStageConfig(Context.DifficultyStage)){Capacity->EventCapacity=C->Budget.ConcurrentPopulation;Capacity->GlobalCapacity=FMath::Max(C->Budget.ConcurrentPopulation,C->Budget.ConcurrentPopulation*2);}}
void URaidDirectorSubsystem::ProcessWaveTick(float Dt)
{
    if(!CurrentPlan||!WaveStateMachine||!CurrentPlan->Waves.IsValidIndex(WaveStateMachine->CurrentWaveIndex))return;
    const int32 PreviousIndex=WaveStateMachine->CurrentWaveIndex;
    const FWavePlan&Wave=CurrentPlan->Waves[PreviousIndex];
    WaveStateMachine->IntermissionDuration=Wave.Intermission;
    WaveStateMachine->Tick(GetWorld()->GetTimeSeconds(),Budget->GetRemainingBudget(),Counter->GetLiveCount(),Wave.Duration);
    if(WaveStateMachine->CurrentState==EWaveState::ProductionClosed||WaveStateMachine->CurrentState==EWaveState::Intermission)
    {
        Lifecycle->SetPhase(Context,ERaidLifecycleState::Intermission);
    }
    if(WaveStateMachine->CurrentWaveIndex!=PreviousIndex)
    {
        const int32 NewIndex=WaveStateMachine->CurrentWaveIndex;
        if(CurrentPlan->Waves.IsValidIndex(NewIndex))
        {
            const FWavePlan&Next=CurrentPlan->Waves[NewIndex];Budget->Initialize(Next.TotalCount);
            Capacity->WaveCapacity=Next.MaxAlive;BudgetWaveIndex=NewIndex;
            OnWaveChanged.Broadcast(NewIndex,CurrentPlan->Waves.Num());
            FRaidFeedbackEvent E;E.Type=Context.EventType;E.SourceModule=TEXT("WaveState");E.TargetModule=TEXT("Budget");
            E.NumericData.Add(TEXT("WaveIndex"),(float)NewIndex);FeedbackBus->Publish(TEXT("WaveChanged"),E);
        }
    }
    if(WaveStateMachine->CurrentState==EWaveState::Producing&&Context.Lifecycle==ERaidLifecycleState::Intermission)
    {
        Lifecycle->SetPhase(Context,ERaidLifecycleState::Active);
    }
    if(WaveStateMachine->IsComplete())EndRaid();
}
void URaidDirectorSubsystem::ProcessSpawns(float Dt){if(!CurrentPlan||WaveStateMachine->CurrentState!=EWaveState::Producing||!EnemyGroups||!Enemies)return;const int32 WI=WaveStateMachine->CurrentWaveIndex;if(!CurrentPlan->Waves.IsValidIndex(WI))return;const FWavePlan&Wave=CurrentPlan->Waves[WI];Capacity->WaveCapacity=Wave.MaxAlive;const int32 Free=Capacity->ComputeFreeCapacity(Counter->GetLiveCount());int32 Request=Limiter->ComputeSpawnRequest(Budget->GetRemainingBudget(),Free);/* FrameBudget and TimeBudget enforce per-frame work limits; detailed rate shaping is a follow-up tuning pass. */if(Request<=0)return;const TArray<FWeightedEntry> Pool=UHordeGroupResolver::ResolveGroup(Wave.GroupID,EnemyGroups);if(Pool.IsEmpty())return;FVector Target=FVector::ZeroVector;for(const FPlayerStateSnapshot&P:Context.Players)if(P.bIsAlive){Target=P.Location;break;}for(int32 I=0;I<Request&&TimeBudget->HasTimeRemaining();++I){if(!FrameBudget->TryConsumeSpawn())break;const FName EnemyID=UHordeCompositionPicker::PickWeightedEnemy(Pool);const FEnemyDefinition*Def=Enemies->FindEnemy(EnemyID);if(!Def||!Def->EnemyClass)continue;const FVector Pos=Finder->FindValidSpawnPosition(GetWorld(),Target);if(Pos.IsZero()||!Finder->IsPositionValid(GetWorld(),Pos))continue;FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;AActor*Enemy=GetWorld()->SpawnActor<AActor>(Def->EnemyClass,Pos,FRotator::ZeroRotator,Params);if(!Enemy)continue;Budget->OnSpawned();Counter->OnEntitySpawned(Enemy,EnemyID);ManagedEnemies.AddUnique(Enemy);Enemy->OnDestroyed.AddDynamic(this,&URaidDirectorSubsystem::HandleEnemyDestroyed);OnEnemySpawned.Broadcast(Enemy);if(Context.Lifecycle!=ERaidLifecycleState::Active)Lifecycle->SetPhase(Context,ERaidLifecycleState::Active);}}

bool URaidDirectorSubsystem::ValidateCurrentPlan(FString& OutError) const
{
    if(!CurrentPlan||!EnemyGroups||!Enemies)
    {
        OutError=TEXT("Plan, enemy-group library, or enemy library is missing.");return false;
    }
    if(!CurrentPlan->ValidatePlan(OutError))return false;
    for(const FWavePlan& Wave:CurrentPlan->Waves)
    {
        const FEnemyGroupDefinition* Group=EnemyGroups->FindGroup(Wave.GroupID);
        if(!Group||Group->Members.IsEmpty())
        {
            OutError=FString::Printf(TEXT("Wave references missing or empty enemy group '%s'."),*Wave.GroupID.ToString());return false;
        }
        bool bHasSpawnableMember=false;
        for(const FWeightedEntry& Member:Group->Members)
        {
            if(!FMath::IsFinite(Member.Weight)||Member.Weight<=0.f)continue;
            const FEnemyDefinition* Definition=Enemies->FindEnemy(Member.EnemyID);
            if(Definition&&Definition->EnemyClass)bHasSpawnableMember=true;
        }
        if(!bHasSpawnableMember)
        {
            OutError=FString::Printf(TEXT("Enemy group '%s' has no valid weighted spawnable members."),*Wave.GroupID.ToString());return false;
        }
    }
    OutError.Empty();return true;
}

bool URaidDirectorSubsystem::HasNavigationData() const
{
    UWorld* World=GetWorld();
    UNavigationSystemV1* Navigation=World?FNavigationSystem::GetCurrent<UNavigationSystemV1>(World):nullptr;
    return Navigation&&Navigation->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)!=nullptr;
}

bool URaidDirectorSubsystem::FindPathForEnemy(AActor* Mover,const FVector& Destination,TArray<FVector>& OutPath)
{
    OutPath.Reset();
    UWorld* World=GetWorld();
    if(!IsValid(Mover)||!World||!HasNavigationData()||!FrameBudget||!FrameBudget->TryConsumePathfind())return false;
    if(Destination.ContainsNaN())return false;
    UNavigationPath* Path=UNavigationSystemV1::FindPathToLocationSynchronously(World,Mover->GetActorLocation(),Destination,Mover);
    if(!Path||!Path->IsValid()||Path->PathPoints.Num()<2||Path->PathPoints.Num()>256)return false;
    for(const FVector& Point:Path->PathPoints)
    {
        if(Point.ContainsNaN())return false;
    }
    OutPath=Path->PathPoints;
    return true;
}

void URaidDirectorSubsystem::CleanupManagedEnemies()
{
    TArray<TObjectPtr<AActor>> ToDestroy=ManagedEnemies;
    ManagedEnemies.Reset();
    for(AActor* Enemy:ToDestroy)
    {
        if(!IsValid(Enemy))continue;
        Enemy->OnDestroyed.RemoveDynamic(this,&URaidDirectorSubsystem::HandleEnemyDestroyed);
        if(Counter)Counter->OnEntityRemoved(Enemy);
        if(!Enemy->IsActorBeingDestroyed())Enemy->Destroy();
    }
}

void URaidDirectorSubsystem::HandleEnemyDestroyed(AActor* Destroyed)
{
    if(Counter)Counter->OnEntityRemoved(Destroyed);
    ManagedEnemies.RemoveAll([Destroyed](const TObjectPtr<AActor>& Item){return !Item||Item==Destroyed;});
    OnEnemyDied.Broadcast(Destroyed);
}
