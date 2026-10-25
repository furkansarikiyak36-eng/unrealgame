#include "StickInventoryComponent.h"

UStickInventoryComponent::UStickInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

int32 UStickInventoryComponent::AddSticks(FName Type, int32 Count)
{
    if (Type.IsNone() || Count <= 0)
    {
        return 0;
    }

    int32 Total = 0;
    for (const FStickBundle& Bundle : Bundles)
    {
        Total += FMath::Max(0, Bundle.Count);
    }

    int32 Remaining = FMath::Min(Count, FMath::Max(0, GlobalCapacity - Total));
    const int32 Accepted = Remaining;

    for (FStickBundle& Bundle : Bundles)
    {
        if (Bundle.StickType != Type || Remaining <= 0)
        {
            continue;
        }

        const int32 Add = FMath::Min(Remaining, FMath::Max(0, Bundle.MaxCount - Bundle.Count));
        Bundle.Count += Add;
        Remaining -= Add;
    }

    while (Remaining > 0)
    {
        FStickBundle Bundle;
        Bundle.StickType = Type;
        Bundle.Count = FMath::Min(Remaining, 20);
        Bundle.MaxCount = 20;
        Bundles.Add(Bundle);
        Remaining -= Bundle.Count;
    }

    return Accepted;
}

int32 UStickInventoryComponent::RemoveSticks(FName Type, int32 Count)
{
    if (Type.IsNone() || Count <= 0)
    {
        return 0;
    }

    int32 Removed = 0;
    for (int32 Index = Bundles.Num() - 1; Index >= 0 && Removed < Count; --Index)
    {
        if (Bundles[Index].StickType != Type)
        {
            continue;
        }

        const int32 Amount = FMath::Min(FMath::Max(0, Bundles[Index].Count), Count - Removed);
        Bundles[Index].Count -= Amount;
        Removed += Amount;
        if (Bundles[Index].Count <= 0)
        {
            Bundles.RemoveAt(Index);
        }
    }

    return Removed;
}

int32 UStickInventoryComponent::GetCount(FName Type) const
{
    int32 Count = 0;
    for (const FStickBundle& Bundle : Bundles)
    {
        if (Bundle.StickType == Type)
        {
            Count += FMath::Max(0, Bundle.Count);
        }
    }
    return Count;
}

bool UStickInventoryComponent::CanApplyCraftTransaction(
    const TArray<FStickBundle>& RequiredInputs, FName OutputType, int32 OutputCount) const
{
    if (OutputType.IsNone() || OutputCount <= 0)
    {
        return false;
    }

    TMap<FName, int32> AggregatedRequirements;
    int64 TotalRequired = 0;

    for (const FStickBundle& Input : RequiredInputs)
    {
        if (Input.StickType.IsNone() || Input.Count <= 0)
        {
            return false;
        }

        int32& RequiredForType = AggregatedRequirements.FindOrAdd(Input.StickType);
        if (Input.Count > MAX_int32 - RequiredForType)
        {
            return false;
        }

        RequiredForType += Input.Count;
        TotalRequired += Input.Count;
    }

    for (const TPair<FName, int32>& Requirement : AggregatedRequirements)
    {
        if (GetCount(Requirement.Key) < Requirement.Value)
        {
            return false;
        }
    }

    int64 TotalCurrent = 0;
    for (const FStickBundle& Bundle : Bundles)
    {
        TotalCurrent += FMath::Max(0, Bundle.Count);
    }

    if (TotalRequired > TotalCurrent)
    {
        return false;
    }

    // Inputs free capacity before output is inserted. This also supports recipes whose
    // output is the same item type as one of their inputs.
    const int64 TotalAfterCraft = TotalCurrent - TotalRequired + OutputCount;
    return TotalAfterCraft <= FMath::Max(0, GlobalCapacity);
}

bool UStickInventoryComponent::ApplyCraftTransaction(
    const TArray<FStickBundle>& RequiredInputs, FName OutputType, int32 OutputCount)
{
    if (!CanApplyCraftTransaction(RequiredInputs, OutputType, OutputCount))
    {
        return false;
    }

    TMap<FName, int32> AggregatedRequirements;
    for (const FStickBundle& Input : RequiredInputs)
    {
        int32& RequiredForType = AggregatedRequirements.FindOrAdd(Input.StickType);
        RequiredForType += Input.Count;
    }

    const TArray<FStickBundle> OriginalBundles = Bundles;
    for (const TPair<FName, int32>& Requirement : AggregatedRequirements)
    {
        if (RemoveSticks(Requirement.Key, Requirement.Value) != Requirement.Value)
        {
            Bundles = OriginalBundles;
            return false;
        }
    }

    if (AddSticks(OutputType, OutputCount) != OutputCount)
    {
        // Unexpected capacity/state changes must not consume the original ingredients.
        Bundles = OriginalBundles;
        return false;
    }

    return true;
}
