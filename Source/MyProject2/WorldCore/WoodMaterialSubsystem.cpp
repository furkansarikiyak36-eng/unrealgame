#include "WoodMaterialSubsystem.h"

#include "CarvableLog.h"
#include "WorldCore/WorldStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UWoodMaterialSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UWorldStateSubsystem>();
    if (const UWorldStateSubsystem* WorldState = GetWorldState())
    {
        LastCompletedWorldDay = WorldState->GetWorldTime().ToTotalDays();
    }
    RefreshTrackedLogs();
}

void UWoodMaterialSubsystem::Deinitialize()
{
    TrackedLogs.Reset();
    BatchCursor = 0;
    bBatchActive = false;
    Super::Deinitialize();
}

UWorldStateSubsystem* UWoodMaterialSubsystem::GetWorldState() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UWorldStateSubsystem>() : nullptr;
}

bool UWoodMaterialSubsystem::IsTickable() const
{
    return !IsTemplate() && GetWorld() != nullptr;
}

TStatId UWoodMaterialSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UWoodMaterialSubsystem, STATGROUP_Tickables);
}

void UWoodMaterialSubsystem::RefreshTrackedLogs()
{
    UWorld* World = GetWorld();
    if (!World || bBatchActive)
    {
        return;
    }

    TrackedLogs.Reset();
    for (TActorIterator<ACarvableLog> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            TrackedLogs.Add(*It);
        }
    }

    // Stable processing order makes the material pass deterministic for a fixed actor set.
    TrackedLogs.Sort([](const TWeakObjectPtr<ACarvableLog>& A, const TWeakObjectPtr<ACarvableLog>& B)
    {
        const ACarvableLog* Left = A.Get();
        const ACarvableLog* Right = B.Get();
        if (!Left) return Right != nullptr;
        if (!Right) return false;
        return Left->GetPathName() < Right->GetPathName();
    });
    BatchCursor = 0;
}

bool UWoodMaterialSubsystem::IsExposedToRain(const ACarvableLog* Log) const
{
    UWorld* World = GetWorld();
    if (!World || !IsValid(Log))
    {
        return false;
    }

    const FVector Start = Log->GetActorLocation() + FVector(0.0f, 0.0f, Log->GetRadius() + 3.0f);
    const FVector End = Start + FVector(0.0f, 0.0f, 50000.0f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WoodRainExposure), false, Log);
    FHitResult Hit;
    return !World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
}

void UWoodMaterialSubsystem::BeginWeatherBatch(float TargetWorldDay, UWorldStateSubsystem* WorldState)
{
    if (!WorldState || bBatchActive || TargetWorldDay <= LastCompletedWorldDay + KINDA_SMALL_NUMBER)
    {
        return;
    }

    if (TrackedLogs.IsEmpty())
    {
        LastCompletedWorldDay = TargetWorldDay;
        return;
    }

    bBatchActive = true;
    BatchCursor = 0;
    LastProcessedLogCount = 0;
    BatchTargetWorldDay = TargetWorldDay;
    BatchElapsedDays = FMath::Min(TargetWorldDay - LastCompletedWorldDay, 365.0f);
    BatchTemperatureC = WorldState->TemperatureC;
    BatchRainfallMm = FMath::Max(0.0f, WorldState->RainfallMm);
    bBatchIsRaining = WorldState->bIsRaining || WorldState->bIsSnowing;
}

void UWoodMaterialSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    UWorldStateSubsystem* WorldState = GetWorldState();
    if (!World || !WorldState)
    {
        return;
    }

    const double NowSeconds = FPlatformTime::Seconds();
    if (!bBatchActive &&
        (LastRefreshTimeSeconds < 0.0 ||
         NowSeconds - LastRefreshTimeSeconds >= FMath::Max(0.25f, RefreshIntervalSeconds)))
    {
        RefreshTrackedLogs();
        LastRefreshTimeSeconds = NowSeconds;
    }

    const float CurrentWorldDay = WorldState->GetWorldTime().ToTotalDays();
    BeginWeatherBatch(CurrentWorldDay, WorldState);
    if (!bBatchActive)
    {
        return;
    }

    const int32 Budget = FMath::Clamp(MaxLogsProcessedPerTick, 1, 512);
    int32 ProcessedThisTick = 0;
    while (BatchCursor < TrackedLogs.Num() && ProcessedThisTick < Budget)
    {
        ACarvableLog* Log = TrackedLogs[BatchCursor++].Get();
        ++ProcessedThisTick;
        if (!IsValid(Log))
        {
            continue;
        }

        const bool bRainExposure = bBatchIsRaining && BatchRainfallMm > 0.0f && IsExposedToRain(Log);
        Log->SimulateMaterialEnvironment(
            BatchElapsedDays, BatchTemperatureC, bRainExposure, BatchRainfallMm);
        ++LastProcessedLogCount;
    }

    if (BatchCursor >= TrackedLogs.Num())
    {
        LastCompletedWorldDay = BatchTargetWorldDay;
        bBatchActive = false;
        BatchCursor = 0;
        // Include actors created while the previous bounded batch was being processed.
        LastRefreshTimeSeconds = -1.0;
    }
}
