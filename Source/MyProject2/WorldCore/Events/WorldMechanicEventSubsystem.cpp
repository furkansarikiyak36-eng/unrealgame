#include "WorldMechanicEventSubsystem.h"

#include "Engine/World.h"

void UWorldMechanicEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PendingEvents.Reserve(FMath::Clamp(MaxQueuedEvents, 64, 65536));
}

void UWorldMechanicEventSubsystem::Deinitialize()
{
    PendingEvents.Reset();
    OnWorldMechanicEvent.Clear();
    Super::Deinitialize();
}

TStatId UWorldMechanicEventSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UWorldMechanicEventSubsystem, STATGROUP_Tickables);
}

bool UWorldMechanicEventSubsystem::IsTickable() const
{
    return GetWorld() != nullptr && !GetWorld()->IsPreviewWorld();
}

bool UWorldMechanicEventSubsystem::EnqueueEvent(FWorldMechanicEvent Event)
{
    if (!Event.IsValid())
    {
        return false;
    }

    if (!Event.EventId.IsValid())
    {
        Event.EventId = FGuid::NewGuid();
    }

    if (Event.Type == EWorldMechanicEventType::VoxelEditCommitted
        || Event.Type == EWorldMechanicEventType::VoxelEditRolledBack
        || Event.Type == EWorldMechanicEventType::VoxelWorldReset
        || Event.Type == EWorldMechanicEventType::PlacementCommitted
        || Event.Type == EWorldMechanicEventType::ShoringPlanCommitted
        || Event.Type == EWorldMechanicEventType::StructureBroken
        || Event.Type == EWorldMechanicEventType::WorldLoadCompleted)
    {
        ++WorldVersion;
    }
    Event.WorldVersion = WorldVersion;

    const int32 QueueLimit = FMath::Clamp(MaxQueuedEvents, 64, 65536);
    if (PendingEvents.Num() >= QueueLimit)
    {
        // Critical events displace the oldest non-critical event; otherwise reject safely.
        if (!Event.bHighPriority)
        {
            return false;
        }
        const int32 ReplaceIndex = PendingEvents.IndexOfByPredicate(
            [](const FWorldMechanicEvent& Pending) { return !Pending.bHighPriority; });
        if (ReplaceIndex == INDEX_NONE)
        {
            return false;
        }
        PendingEvents.RemoveAt(ReplaceIndex, 1, EAllowShrinking::No);
    }

    if (Event.bHighPriority)
    {
        PendingEvents.Insert(MoveTemp(Event), 0);
    }
    else
    {
        PendingEvents.Add(MoveTemp(Event));
    }
    return true;
}

bool UWorldMechanicEventSubsystem::PublishImmediate(FWorldMechanicEvent Event)
{
    if (!Event.IsValid())
    {
        return false;
    }
    if (!Event.EventId.IsValid()) Event.EventId = FGuid::NewGuid();
    Event.WorldVersion = WorldVersion;
    Dispatch(Event);
    return true;
}

void UWorldMechanicEventSubsystem::Tick(float DeltaTime)
{
    const int32 Limit = FMath::Clamp(MaxEventsPerTick, 1, 256);
    const int32 Count = FMath::Min(Limit, PendingEvents.Num());
    for (int32 Index = 0; Index < Count; ++Index)
    {
        FWorldMechanicEvent Event = MoveTemp(PendingEvents[0]);
        PendingEvents.RemoveAt(0, 1, EAllowShrinking::No);
        Dispatch(Event);
    }
}

void UWorldMechanicEventSubsystem::ClearTransientEvents()
{
    PendingEvents.Reset();
}

void UWorldMechanicEventSubsystem::Dispatch(const FWorldMechanicEvent& Event)
{
    OnWorldMechanicEvent.Broadcast(Event);
}
