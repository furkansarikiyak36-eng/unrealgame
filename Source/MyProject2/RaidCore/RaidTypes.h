#pragma once
#include "CoreMinimal.h"
#include "RaidTypes.generated.h"

UENUM(BlueprintType) enum class ERaidEventType : uint8 { None, BloodMoon, HordeNight, SiegeEvent, ScriptedAssault };
UENUM(BlueprintType) enum class ERaidLifecycleState : uint8 { Idle, Scheduling, Preparing, Active, Intermission, Ending, Completed, Cancelled };
UENUM(BlueprintType) enum class ERaidRequestType : uint8 { None, Spawn, Pathfind, TargetUpdate, DefenseHit, WaveTransition };
UENUM(BlueprintType) enum class EEventConflictPolicy : uint8 { Parallel, Serial, Defer, Priority };
UENUM(BlueprintType) enum class EAlternativeAction : uint8 { None, RecalculatePath, FindAccessPoint, AttackObstacle, Wait, ChangeMovement };
UENUM(BlueprintType) enum class EWaveState : uint8 { Preparation, Producing, ProductionClosed, Intermission, NextWave, Completed };
UENUM(BlueprintType) enum class EObstacleType : uint8 { Passable, Openable, Breakable, Blocking, Structural };

USTRUCT(BlueprintType) struct FPlayerStateSnapshot { GENERATED_BODY() UPROPERTY(BlueprintReadWrite) int32 PlayerId=-1; UPROPERTY(BlueprintReadWrite) int32 Level=1; UPROPERTY(BlueprintReadWrite) float SurvivalTime=0.f; UPROPERTY(BlueprintReadWrite) int32 DeathCount=0; UPROPERTY(BlueprintReadWrite) FVector Location=FVector::ZeroVector; UPROPERTY(BlueprintReadWrite) bool bIsAlive=true; };
USTRUCT(BlueprintType) struct FWorldSnapshot { GENERATED_BODY() UPROPERTY(BlueprintReadWrite) int32 BiomeDifficulty=1; UPROPERTY(BlueprintReadWrite) float WorldMultiplier=1.f; UPROPERTY(BlueprintReadWrite) float InGameTime=0.f; UPROPERTY(BlueprintReadWrite) bool bIsNight=false; };
USTRUCT(BlueprintType) struct FRaidMultiplier { GENERATED_BODY() UPROPERTY(BlueprintReadWrite) FName Tag; UPROPERTY(BlueprintReadWrite) float Value=1.f; UPROPERTY(BlueprintReadWrite) int32 Order=0; };
USTRUCT(BlueprintType) struct FWeightedEntry { GENERATED_BODY() UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EnemyID; UPROPERTY(EditAnywhere, BlueprintReadWrite) float Weight=1.f; };
USTRUCT(BlueprintType) struct FConditionalEntry { GENERATED_BODY() UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EnemyID; UPROPERTY(EditAnywhere, BlueprintReadWrite) float Weight=1.f; UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MinStage=0; UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxStage=999; UPROPERTY(EditAnywhere, BlueprintReadWrite) FName RequiredTag; };
USTRUCT(BlueprintType) struct FEnemyDefinition { GENERATED_BODY() UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EnemyID; UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AActor> EnemyClass; UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 BaseHealth=100; UPROPERTY(EditAnywhere, BlueprintReadWrite) float BaseDamage=10.f; UPROPERTY(EditAnywhere, BlueprintReadWrite) float MoveSpeed=300.f; UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ThreatLevel=1; };
USTRUCT(BlueprintType) struct FEnemyGroupDefinition { GENERATED_BODY() UPROPERTY(EditAnywhere, BlueprintReadWrite) FName GroupID; UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FWeightedEntry> Members; };
USTRUCT(BlueprintType) struct FWavePlan { GENERATED_BODY() UPROPERTY(EditAnywhere, BlueprintReadWrite) FName GroupID; UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TotalCount=20; UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxAlive=5; UPROPERTY(EditAnywhere, BlueprintReadWrite) float Duration=60.f; UPROPERTY(EditAnywhere, BlueprintReadWrite) float Intermission=10.f; };
USTRUCT(BlueprintType) struct FHordePlan { GENERATED_BODY() UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PlanID; UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 StageIndex=0; UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FWavePlan> Waves; UPROPERTY(EditAnywhere, BlueprintReadWrite) float TotalDuration=0.f; UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bUseStationKeeping=false; UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 StationKeepingMaxAlive=3; };
USTRUCT(BlueprintType) struct FDifficultyBudget { GENERATED_BODY() UPROPERTY(BlueprintReadWrite) int32 MinEnemyLevel=1; UPROPERTY(BlueprintReadWrite) int32 MaxEnemyLevel=5; UPROPERTY(BlueprintReadWrite) int32 ConcurrentPopulation=8; UPROPERTY(BlueprintReadWrite) int32 TotalBudget=60; UPROPERTY(BlueprintReadWrite) float RaidDuration=180.f; };
USTRUCT(BlueprintType) struct FSpawnRequest { GENERATED_BODY() UPROPERTY(BlueprintReadWrite) FName EnemyID; UPROPERTY(BlueprintReadWrite) FVector TargetLocation=FVector::ZeroVector; UPROPERTY(BlueprintReadWrite) float Priority=0.f; UPROPERTY(BlueprintReadWrite) int32 WaveIndex=0; UPROPERTY(BlueprintReadWrite) double RequestTime=0.0; };
USTRUCT(BlueprintType) struct FRetryPolicy { GENERATED_BODY() UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxAttempts=5; UPROPERTY(EditAnywhere, BlueprintReadWrite) float AttemptInterval=.2f; UPROPERTY(EditAnywhere, BlueprintReadWrite) float BackoffMultiplier=1.5f; };
USTRUCT(BlueprintType) struct FTargetCandidate { GENERATED_BODY() UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> Target; UPROPERTY(BlueprintReadWrite) float Score=0.f; UPROPERTY(BlueprintReadWrite) float Distance=0.f; UPROPERTY(BlueprintReadWrite) bool bVisible=false; };
USTRUCT(BlueprintType) struct FNavNode { GENERATED_BODY() UPROPERTY(BlueprintReadWrite) FVector Position=FVector::ZeroVector; UPROPERTY(BlueprintReadWrite) TArray<int32> Neighbors; UPROPERTY(BlueprintReadWrite) float BaseCost=1.f; };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaidEventStarted, FName, EventID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaidEventEnded, FName, EventID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRaidWaveChanged, int32, WaveIndex, int32, TotalWaves);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaidEnemySpawned, AActor*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaidEnemyDied, AActor*, Enemy);
