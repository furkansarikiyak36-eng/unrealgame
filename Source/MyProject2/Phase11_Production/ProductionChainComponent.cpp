#include "ProductionChainComponent.h"

#include "ProductionRecipeAsset.h"
#include "InventoryCore/InventoryComponent.h"
#include "InventoryCore/InventoryStorageActor.h"
#include "ForestCharacter.h"
#include "WorldCore/WorldPersistenceSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
    bool ScaleAmount(int32 Amount, int32 BatchCount, int32& OutScaled)
    {
        if (Amount <= 0 || BatchCount <= 0)
        {
            return false;
        }
        const int64 Scaled = static_cast<int64>(Amount) * static_cast<int64>(BatchCount);
        if (Scaled <= 0 || Scaled > TNumericLimits<int32>::Max())
        {
            return false;
        }
        OutScaled = static_cast<int32>(Scaled);
        return true;
    }

    FInventoryItem MakeInventoryPayload(FName ItemID, int32 Count)
    {
        FInventoryItem Item;
        Item.ItemID = ItemID;
        Item.Count = Count;
        return Item;
    }
}

UProductionChainComponent::UProductionChainComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.05f;
}

void UProductionChainComponent::BeginPlay()
{
    Super::BeginPlay();
    QueueCapacity = FMath::Clamp(QueueCapacity, 1, 32);
    MaxRetainedJobs = FMath::Clamp(MaxRetainedJobs, QueueCapacity, 128);
    AvailablePowerWatts = FMath::IsFinite(AvailablePowerWatts) ? FMath::Max(0.0f, AvailablePowerWatts) : 0.0f;
    AvailableWorkers = FMath::Clamp(AvailableWorkers, 0, 1000);
    bQueuePaused = bQueueStartsPaused;
}

FName UProductionChainComponent::GetStableStationKey() const
{
    if (!StationKey.IsNone())
    {
        return StationKey;
    }
    const AActor* Owner = GetOwner();
    return Owner ? Owner->GetFName() : NAME_None;
}

int32 UProductionChainComponent::GetActiveJobCount() const
{
    int32 Count = 0;
    for (const FProductionJob& Job : Jobs)
    {
        if (IsActiveStatus(Job.Status))
        {
            ++Count;
        }
    }
    return Count;
}

bool UProductionChainComponent::IsActiveStatus(EProductionJobStatus Status)
{
    return Status == EProductionJobStatus::Queued
        || Status == EProductionJobStatus::Running
        || Status == EProductionJobStatus::BlockedPower
        || Status == EProductionJobStatus::BlockedWorkers
        || Status == EProductionJobStatus::BlockedOutput;
}

UInventoryComponent* UProductionChainComponent::ResolveInputInventory() const
{
    if (IsValid(InputInventory))
    {
        return InputInventory;
    }
    AActor* Owner = GetOwner();
    return Owner ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
}

UInventoryComponent* UProductionChainComponent::ResolveOutputInventory() const
{
    if (IsValid(OutputInventory))
    {
        return OutputInventory;
    }
    return ResolveInputInventory();
}

UProductionRecipeAsset* UProductionChainComponent::FindUniqueRecipe(FName InRecipeID) const
{
    if (InRecipeID.IsNone())
    {
        return nullptr;
    }

    UProductionRecipeAsset* Match = nullptr;
    for (UProductionRecipeAsset* Recipe : Recipes)
    {
        if (!IsValid(Recipe) || Recipe->RecipeID != InRecipeID)
        {
            continue;
        }
        if (Match)
        {
            return nullptr;
        }
        Match = Recipe;
    }
    return Match;
}

bool UProductionChainComponent::QueueRecipeByID(FName RecipeID, int32 BatchCount)
{
    UProductionRecipeAsset* Recipe = FindUniqueRecipe(RecipeID);
    return Recipe && QueueRecipe(Recipe, BatchCount);
}

bool UProductionChainComponent::QueueConfiguredRecipe(
    FName RecipeID,
    const TArray<FProductionIngredient>& Inputs,
    const TArray<FProductionOutput>& Outputs,
    float DurationSeconds,
    int32 BatchCount,
    float InRequiredPowerWatts,
    int32 InRequiredWorkers)
{
    if (RecipeID.IsNone())
    {
        return false;
    }
    return EnqueuePlan(RecipeID, FText::FromName(RecipeID), Inputs, Outputs,
        DurationSeconds, InRequiredPowerWatts, InRequiredWorkers, BatchCount);
}

bool UProductionChainComponent::QueueRecipe(UProductionRecipeAsset* Recipe, int32 BatchCount)
{
    if (!IsValid(Recipe))
    {
        return false;
    }
    FString ValidationError;
    if (!Recipe->ValidateRecipe(ValidationError))
    {
        UE_LOG(LogTemp, Warning, TEXT("Production recipe rejected at '%s': %s"),
            *GetNameSafe(GetOwner()), *ValidationError);
        return false;
    }
    return EnqueuePlan(Recipe->RecipeID, Recipe->DisplayName, Recipe->Inputs, Recipe->Outputs,
        Recipe->BaseDurationSeconds, Recipe->RequiredPowerWatts, Recipe->RequiredWorkers, BatchCount);
}

bool UProductionChainComponent::QueueStandardPlankBatch(int32 BatchCount)
{
    TArray<FProductionIngredient> Inputs;
    FProductionIngredient Input;
    Input.ItemID = FName(TEXT("HalfLog"));
    Input.Count = 1;
    Inputs.Add(Input);

    TArray<FProductionOutput> Outputs;
    FProductionOutput Output;
    Output.ItemID = FName(TEXT("Plank"));
    Output.Count = 2;
    Outputs.Add(Output);

    return EnqueuePlan(FName(TEXT("PlankFromHalfLog")), FText::FromString(TEXT("Half-log to planks")),
        Inputs, Outputs, 5.0f, 100.0f, 1, BatchCount);
}

bool UProductionChainComponent::EnqueuePlan(
    FName RecipeID,
    const FText& DisplayName,
    const TArray<FProductionIngredient>& Inputs,
    const TArray<FProductionOutput>& Outputs,
    float BaseDurationSeconds,
    float RequiredPowerWatts,
    int32 RequiredWorkers,
    int32 BatchCount)
{
    if (GetStableStationKey().IsNone() || RecipeID.IsNone()
        || BatchCount < 1 || BatchCount > MaxBatchCount
        || Inputs.IsEmpty() || Outputs.IsEmpty()
        || !FMath::IsFinite(BaseDurationSeconds) || BaseDurationSeconds < 0.01f || BaseDurationSeconds > 86400.0f
        || !FMath::IsFinite(RequiredPowerWatts) || RequiredPowerWatts < 0.0f || RequiredPowerWatts > 100000000.0f
        || RequiredWorkers < 0 || RequiredWorkers > 1000)
    {
        return false;
    }
    const float TotalDuration = BaseDurationSeconds * static_cast<float>(BatchCount);
    if (!FMath::IsFinite(TotalDuration) || TotalDuration > 8640000.0f)
    {
        return false;
    }

    TrimCompletedHistory();
    if (GetActiveJobCount() >= FMath::Clamp(QueueCapacity, 1, 32) || Jobs.Num() >= FMath::Clamp(MaxRetainedJobs, 1, 128))
    {
        return false;
    }

    TArray<FInventoryItem> ScaledInputs;
    TArray<FInventoryItem> ScaledOutputs;
    ScaledInputs.Reserve(Inputs.Num());
    ScaledOutputs.Reserve(Outputs.Num());
    for (const FProductionIngredient& Input : Inputs)
    {
        int32 Count = 0;
        if (Input.ItemID.IsNone() || !ScaleAmount(Input.Count, BatchCount, Count))
        {
            return false;
        }
        ScaledInputs.Add(MakeInventoryPayload(Input.ItemID, Count));
    }
    for (const FProductionOutput& Output : Outputs)
    {
        int32 Count = 0;
        if (Output.ItemID.IsNone() || !ScaleAmount(Output.Count, BatchCount, Count))
        {
            return false;
        }
        ScaledOutputs.Add(MakeInventoryPayload(Output.ItemID, Count));
    }

    UInventoryComponent* InputStore = ResolveInputInventory();
    if (!IsValid(InputStore) || !UInventoryComponent::ExchangeItemBatches(InputStore, nullptr, ScaledInputs, {}))
    {
        return false;
    }

    FProductionJob NewJob;
    NewJob.JobId = FGuid::NewGuid();
    NewJob.RecipeID = RecipeID;
    NewJob.DisplayName = DisplayName.IsEmpty() ? FText::FromName(RecipeID) : DisplayName;
    NewJob.ReservedInputs = MoveTemp(ScaledInputs);
    NewJob.Outputs = MoveTemp(ScaledOutputs);
    NewJob.BatchCount = BatchCount;
    NewJob.RequiredDurationSeconds = TotalDuration;
    NewJob.RequiredPowerWatts = RequiredPowerWatts;
    NewJob.RequiredWorkers = RequiredWorkers;
    NewJob.Status = EProductionJobStatus::Queued;
    Jobs.Add(MoveTemp(NewJob));
    NotifyStateChanged(Jobs.Last().JobId, Jobs.Last().Status);
    return true;
}

void UProductionChainComponent::TrimCompletedHistory()
{
    const int32 Limit = FMath::Clamp(MaxRetainedJobs, 1, 128);
    while (Jobs.Num() >= Limit)
    {
        const int32 FinishedIndex = Jobs.IndexOfByPredicate([](const FProductionJob& Job)
        {
            return Job.Status == EProductionJobStatus::Completed || Job.Status == EProductionJobStatus::Cancelled;
        });
        if (FinishedIndex == INDEX_NONE)
        {
            break;
        }
        Jobs.RemoveAt(FinishedIndex);
    }
}

bool UProductionChainComponent::CancelJob(const FGuid& JobId)
{
    if (!JobId.IsValid())
    {
        return false;
    }
    const int32 Index = Jobs.IndexOfByPredicate([JobId](const FProductionJob& Job) { return Job.JobId == JobId; });
    if (Index == INDEX_NONE || !IsActiveStatus(Jobs[Index].Status))
    {
        return false;
    }

    FProductionJob& Job = Jobs[Index];
    UInventoryComponent* InputStore = ResolveInputInventory();
    if (!Job.ReservedInputs.IsEmpty()
        && (!IsValid(InputStore) || !UInventoryComponent::ExchangeItemBatches(nullptr, InputStore, {}, Job.ReservedInputs)))
    {
        return false;
    }

    Job.ReservedInputs.Reset();
    Job.Status = EProductionJobStatus::Cancelled;
    NotifyStateChanged(Job.JobId, Job.Status);
    return true;
}

void UProductionChainComponent::PauseQueue()
{
    if (!bQueuePaused)
    {
        bQueuePaused = true;
        NotifyStateChanged();
    }
}

void UProductionChainComponent::ResumeQueue()
{
    if (bQueuePaused)
    {
        bQueuePaused = false;
        NotifyStateChanged();
    }
}

void UProductionChainComponent::ClearCompletedHistory()
{
    const int32 Before = Jobs.Num();
    Jobs.RemoveAll([](const FProductionJob& Job)
    {
        return Job.Status == EProductionJobStatus::Completed || Job.Status == EProductionJobStatus::Cancelled;
    });
    if (Jobs.Num() != Before)
    {
        NotifyStateChanged();
    }
}

bool UProductionChainComponent::SetAvailablePowerWatts(float InAvailablePowerWatts)
{
    if (!FMath::IsFinite(InAvailablePowerWatts) || InAvailablePowerWatts < 0.0f || InAvailablePowerWatts > 100000000.0f)
    {
        return false;
    }
    if (!FMath::IsNearlyEqual(AvailablePowerWatts, InAvailablePowerWatts))
    {
        AvailablePowerWatts = InAvailablePowerWatts;
    }
    return true;
}

bool UProductionChainComponent::SetAvailableWorkers(int32 InAvailableWorkers)
{
    if (InAvailableWorkers < 0 || InAvailableWorkers > 1000)
    {
        return false;
    }
    AvailableWorkers = InAvailableWorkers;
    return true;
}

void UProductionChainComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bQueuePaused)
    {
        return;
    }

    const float SafeDelta = FMath::Clamp(
        FMath::IsFinite(DeltaTime) ? DeltaTime : 0.0f,
        0.0f,
        static_cast<float>(MaxDeltaSecondsPerTick));

    for (FProductionJob& Job : Jobs)
    {
        if (!IsActiveStatus(Job.Status))
        {
            continue;
        }

        if (Job.ProgressSeconds < Job.RequiredDurationSeconds)
        {
            if (AvailablePowerWatts + KINDA_SMALL_NUMBER < Job.RequiredPowerWatts)
            {
                if (Job.Status != EProductionJobStatus::BlockedPower)
                {
                    Job.Status = EProductionJobStatus::BlockedPower;
                    NotifyStateChanged(Job.JobId, Job.Status);
                }
                return;
            }
            if (AvailableWorkers < Job.RequiredWorkers)
            {
                if (Job.Status != EProductionJobStatus::BlockedWorkers)
                {
                    Job.Status = EProductionJobStatus::BlockedWorkers;
                    NotifyStateChanged(Job.JobId, Job.Status);
                }
                return;
            }

            const int32 OldWholeSecond = FMath::FloorToInt(Job.ProgressSeconds);
            Job.ProgressSeconds = FMath::Min(Job.RequiredDurationSeconds, Job.ProgressSeconds + SafeDelta);
            Job.Status = EProductionJobStatus::Running;
            if (FMath::FloorToInt(Job.ProgressSeconds) != OldWholeSecond
                || Job.ProgressSeconds >= Job.RequiredDurationSeconds)
            {
                NotifyStateChanged(Job.JobId, Job.Status);
            }
            if (Job.ProgressSeconds < Job.RequiredDurationSeconds)
            {
                return;
            }
        }

        UInventoryComponent* OutputStore = ResolveOutputInventory();
        if (!IsValid(OutputStore)
            || !UInventoryComponent::ExchangeItemBatches(nullptr, OutputStore, {}, Job.Outputs))
        {
            if (Job.Status != EProductionJobStatus::BlockedOutput)
            {
                Job.Status = EProductionJobStatus::BlockedOutput;
                NotifyStateChanged(Job.JobId, Job.Status);
            }
            return;
        }

        Job.ReservedInputs.Reset();
        Job.Status = EProductionJobStatus::Completed;
        NotifyStateChanged(Job.JobId, Job.Status);
        return; // One output commit per tick prevents queue bursts after a long frame.
    }
}

bool UProductionChainComponent::IsValidSavedJob(const FProductionJob& Job)
{
    if (!Job.JobId.IsValid() || Job.RecipeID.IsNone()
        || Job.BatchCount < 1 || Job.BatchCount > MaxBatchCount
        || !FMath::IsFinite(Job.ProgressSeconds) || Job.ProgressSeconds < 0.0f
        || !FMath::IsFinite(Job.RequiredDurationSeconds) || Job.RequiredDurationSeconds < 0.01f
        || Job.ProgressSeconds > Job.RequiredDurationSeconds
        || !FMath::IsFinite(Job.RequiredPowerWatts) || Job.RequiredPowerWatts < 0.0f
        || Job.RequiredPowerWatts > 100000000.0f || Job.RequiredWorkers < 0 || Job.RequiredWorkers > 1000
        || static_cast<uint8>(Job.Status) > static_cast<uint8>(EProductionJobStatus::Cancelled)
        || Job.Outputs.IsEmpty())
    {
        return false;
    }
    for (const FInventoryItem& Item : Job.Outputs)
    {
        if (Item.ItemID.IsNone() || Item.Count <= 0)
        {
            return false;
        }
    }
    for (const FInventoryItem& Item : Job.ReservedInputs)
    {
        if (Item.ItemID.IsNone() || Item.Count <= 0)
        {
            return false;
        }
    }
    if (IsActiveStatus(Job.Status) && Job.ReservedInputs.IsEmpty())
    {
        return false;
    }
    if ((Job.Status == EProductionJobStatus::Completed || Job.Status == EProductionJobStatus::Cancelled)
        && !Job.ReservedInputs.IsEmpty())
    {
        return false;
    }
    if (Job.Status == EProductionJobStatus::Completed
        && Job.ProgressSeconds + KINDA_SMALL_NUMBER < Job.RequiredDurationSeconds)
    {
        return false;
    }
    return true;
}

bool UProductionChainComponent::ExportSnapshot(FProductionChainSnapshot& OutSnapshot) const
{
    FProductionChainSnapshot Snapshot;
    Snapshot.SchemaVersion = 1;
    Snapshot.StationKey = GetStableStationKey();
    Snapshot.Revision = Revision;
    Snapshot.bQueuePaused = bQueuePaused;
    Snapshot.Jobs = Jobs;
    if (!ValidateSnapshot(Snapshot))
    {
        return false;
    }
    OutSnapshot = MoveTemp(Snapshot);
    return true;
}

bool UProductionChainComponent::UsesCorePersistedInventories() const
{
    const UInventoryComponent* InputStore = ResolveInputInventory();
    const UInventoryComponent* OutputStore = ResolveOutputInventory();
    if (!IsValid(InputStore) || !IsValid(OutputStore))
    {
        return false;
    }
    const AActor* InputOwner = InputStore->GetOwner();
    const AActor* OutputOwner = OutputStore->GetOwner();
    const auto IsCoreSavedOwner = [](const AActor* Owner)
    {
        return IsValid(Owner) && (Cast<AForestCharacter>(Owner) != nullptr || Cast<AInventoryStorageActor>(Owner) != nullptr);
    };
    return IsCoreSavedOwner(InputOwner) && IsCoreSavedOwner(OutputOwner);
}

bool UProductionChainComponent::ValidateSnapshot(const FProductionChainSnapshot& Snapshot) const
{
    if (Snapshot.SchemaVersion != 1 || Snapshot.StationKey.IsNone()
        || Snapshot.StationKey != GetStableStationKey() || Snapshot.Revision < 0
        || Snapshot.Jobs.Num() > FMath::Clamp(MaxRetainedJobs, 1, 128))
    {
        return false;
    }

    TSet<FGuid> JobIds;
    int32 ActiveJobs = 0;
    for (const FProductionJob& Job : Snapshot.Jobs)
    {
        if (!IsValidSavedJob(Job) || JobIds.Contains(Job.JobId))
        {
            return false;
        }
        JobIds.Add(Job.JobId);
        if (IsActiveStatus(Job.Status))
        {
            ++ActiveJobs;
        }
    }
    if (ActiveJobs > FMath::Clamp(QueueCapacity, 1, 32))
    {
        return false;
    }
    if (ActiveJobs > 0 && (!IsValid(ResolveInputInventory()) || !IsValid(ResolveOutputInventory())
        || !UsesCorePersistedInventories()))
    {
        return false;
    }
    return true;
}

bool UProductionChainComponent::ImportSnapshot(const FProductionChainSnapshot& Snapshot)
{
    if (!ValidateSnapshot(Snapshot))
    {
        return false;
    }

    TArray<FProductionJob> NewJobs = Snapshot.Jobs;
    Jobs = MoveTemp(NewJobs);
    bQueuePaused = Snapshot.bQueuePaused;
    Revision = Snapshot.Revision;
    OnQueueChanged.Broadcast(GetStableStationKey());
    return true;
}

void UProductionChainComponent::NotifyStateChanged(const FGuid& JobId, EProductionJobStatus Status)
{
    if (Revision < TNumericLimits<int64>::Max())
    {
        ++Revision;
    }
    if (JobId.IsValid())
    {
        OnJobChanged.Broadcast(JobId, Status);
    }
    OnQueueChanged.Broadcast(GetStableStationKey());
    if (UWorld* World = GetWorld())
    {
        if (UWorldPersistenceSubsystem* Persistence = World->GetSubsystem<UWorldPersistenceSubsystem>())
        {
            Persistence->NotifyProductionChanged(GetStableStationKey());
        }
    }
}
