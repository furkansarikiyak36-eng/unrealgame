#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RaidContext.h"
#include "RaidTypes.h"
#include "RaidDirectorSubsystem.generated.h"
class URaidSchedulerComponent; class URaidDebouncer; class URaidLifecycleComponent;
class URaidStageTableAsset; class UHordePlanAsset; class UEnemyGroupLibrary; class UEnemyLibrary;
class USpawnBudgetTracker; class USpawnCapacityCalculator; class USpawnRequestLimiter; class USpawnPositionFinder; class USpawnLoadBalancer; class UPopulationCounter;
class URaidWaveStateMachine; class URaidFrameBudget; class URaidTimeBudget; class URaidFeedbackBus;
UCLASS() class MYPROJECT2_API URaidDirectorSubsystem : public UTickableWorldSubsystem {
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;
 virtual void Tick(float DeltaTime) override;
 virtual TStatId GetStatId() const override;
 UFUNCTION(BlueprintCallable,Category="Raid") void StartRaid(FName EventID,ERaidEventType Type);
 UFUNCTION(BlueprintCallable,Category="Raid") void EndRaid();
 UFUNCTION(BlueprintCallable,Category="Raid") void CancelRaid(FName Reason);
 UFUNCTION(BlueprintCallable,Category="Raid") void ConfigureRaidAssets(URaidStageTableAsset* InStageTable,const TArray<UHordePlanAsset*>& InPlans,UEnemyGroupLibrary* InGroups,UEnemyLibrary* InEnemies);
 UFUNCTION(BlueprintCallable,Category="Raid|Scheduler") bool SetAutomaticSchedulingEnabled(bool bEnabled,float IntervalSeconds=1800.f,float JitterMinSeconds=-300.f,float JitterMaxSeconds=300.f,ERaidEventType EventType=ERaidEventType::HordeNight);
 UFUNCTION(BlueprintPure,Category="Raid|Navigation") bool HasNavigationData() const;
 UFUNCTION(BlueprintCallable,Category="Raid|Navigation") bool FindPathForEnemy(AActor* Mover,const FVector& Destination,TArray<FVector>& OutPath);
 UFUNCTION(BlueprintPure,Category="Raid|Diagnostics") int32 GetManagedEnemyCount() const { return ManagedEnemies.Num(); }
 const FRaidContext& GetContext() const{return Context;}
 UPROPERTY(BlueprintAssignable) FOnRaidEventStarted OnEventStarted;
 UPROPERTY(BlueprintAssignable) FOnRaidEventEnded OnEventEnded;
 UPROPERTY(BlueprintAssignable) FOnRaidWaveChanged OnWaveChanged;
 UPROPERTY(BlueprintAssignable) FOnRaidEnemySpawned OnEnemySpawned;
 UPROPERTY(BlueprintAssignable) FOnRaidEnemyDied OnEnemyDied;
protected:
 UFUNCTION() void HandleEnemyDestroyed(AActor* Destroyed);
 UPROPERTY() FRaidContext Context;
 UPROPERTY() TObjectPtr<URaidSchedulerComponent> Scheduler;
 UPROPERTY() TObjectPtr<URaidDebouncer> Debouncer;
 UPROPERTY() TObjectPtr<URaidLifecycleComponent> Lifecycle;
 UPROPERTY() TObjectPtr<USpawnBudgetTracker> Budget;
 UPROPERTY() TObjectPtr<USpawnCapacityCalculator> Capacity;
 UPROPERTY() TObjectPtr<USpawnRequestLimiter> Limiter;
 UPROPERTY() TObjectPtr<USpawnPositionFinder> Finder;
 UPROPERTY() TObjectPtr<USpawnLoadBalancer> Balancer;
 UPROPERTY() TObjectPtr<UPopulationCounter> Counter;
 UPROPERTY() TObjectPtr<URaidWaveStateMachine> WaveStateMachine;
 UPROPERTY() TObjectPtr<URaidFrameBudget> FrameBudget;
 UPROPERTY() TObjectPtr<URaidTimeBudget> TimeBudget;
 UPROPERTY() TObjectPtr<URaidFeedbackBus> FeedbackBus;
 /** Actors spawned by this director, never every enemy in the world. */
 UPROPERTY(Transient) TArray<TObjectPtr<AActor>> ManagedEnemies;
 UPROPERTY() TObjectPtr<URaidStageTableAsset> StageTable;
 UPROPERTY() TArray<TObjectPtr<UHordePlanAsset>> AvailablePlans;
 UPROPERTY() TObjectPtr<UEnemyGroupLibrary> EnemyGroups;
 UPROPERTY() TObjectPtr<UEnemyLibrary> Enemies;
 UPROPERTY() TObjectPtr<UHordePlanAsset> CurrentPlan;
 float ElapsedEventTime=0.f;
 float AutoScheduleIntervalSeconds=1800.f;
 float AutoScheduleJitterMinSeconds=-300.f;
 float AutoScheduleJitterMaxSeconds=300.f;
 ERaidEventType AutoScheduleEventType=ERaidEventType::HordeNight;
 bool bAutomaticSchedulingEnabled=false;
 FName AutoScheduleEventID=TEXT("ScheduledHordeNight");
 int32 BudgetWaveIndex=INDEX_NONE;
 int32 EventBudgetRemaining=0;
 void CaptureContext(); void EvaluateDifficulty(); void BuildPlan(); void ProcessSpawns(float DeltaTime); void ProcessWaveTick(float DeltaTime);
 bool ValidateCurrentPlan(FString& OutError) const;
 void CleanupManagedEnemies();
};
