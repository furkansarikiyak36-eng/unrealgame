// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShoringSupportResolver.h"

namespace
{
    constexpr float WarningUtilization = 0.75f;
    constexpr float OverloadUtilization = 1.0f;
    constexpr float MinimumCapacityFactor = 0.10f;
}

float FShoringSupportResolver::GetNominalCapacity(EShoringPieceKind Kind)
{
    switch (Kind)
    {
    case EShoringPieceKind::Upright:  return 2.25f;
    case EShoringPieceKind::Lagging:  return 1.10f;
    case EShoringPieceKind::Strut:    return 2.25f;
    case EShoringPieceKind::BaseSole: return 2.75f;
    case EShoringPieceKind::Platform: return 1.40f;
    case EShoringPieceKind::Ladder:   return 1.10f;
    default:                          return 0.0f;
    }
}

bool FShoringSupportResolver::Evaluate(
    const TArray<FShoringSupportNode>& Nodes,
    TArray<FShoringSupportAssessment>& OutAssessments,
    FString& OutError)
{
    OutAssessments.Reset();
    OutError.Reset();
    if (Nodes.IsEmpty() || Nodes.Num() > 4096)
    {
        OutError = TEXT("Support evaluation requires between 1 and 4096 nodes.");
        return false;
    }

    TMap<FGuid, int32> IndexById;
    IndexById.Reserve(Nodes.Num());
    for (int32 Index = 0; Index < Nodes.Num(); ++Index)
    {
        const FShoringSupportNode& Node = Nodes[Index];
        if (!Node.PieceId.IsValid() || IndexById.Contains(Node.PieceId))
        {
            OutError = TEXT("Support graph contains an invalid or duplicate piece GUID.");
            return false;
        }
        if (!FMath::IsFinite(Node.DesignLoad) || Node.DesignLoad <= 0.0f
            || Node.DesignLoad > 1000.0f || !FMath::IsFinite(Node.HealthFraction)
            || Node.HealthFraction < 0.0f || Node.HealthFraction > 1.0f)
        {
            OutError = TEXT("Support graph contains an invalid design load or health fraction.");
            return false;
        }
        IndexById.Add(Node.PieceId, Index);
    }

    OutAssessments.SetNum(Nodes.Num());
    TArray<bool> bUnsupported;
    TArray<bool> bFailed;
    bUnsupported.Init(false, Nodes.Num());
    bFailed.Init(false, Nodes.Num());

    for (int32 Index = 0; Index < Nodes.Num(); ++Index)
    {
        const FShoringSupportNode& Node = Nodes[Index];
        FShoringSupportAssessment& Assessment = OutAssessments[Index];
        Assessment.PieceId = Node.PieceId;
        Assessment.NominalCapacity = GetNominalCapacity(Node.Kind);
        Assessment.AppliedLoad = Node.DesignLoad;

        if (Assessment.NominalCapacity <= 0.0f)
        {
            OutError = TEXT("Support graph contains an unsupported shoring piece kind.");
            OutAssessments.Reset();
            return false;
        }
        if (Node.HealthFraction <= KINDA_SMALL_NUMBER)
        {
            bFailed[Index] = true;
            Assessment.Condition = EShoringSupportCondition::Failed;
            Assessment.Reason = TEXT("Piece durability is depleted; replace the failed member.");
            continue;
        }
        if (Node.bIsCollapsed)
        {
            bUnsupported[Index] = true;
            Assessment.Condition = EShoringSupportCondition::Unsupported;
            Assessment.Reason = TEXT("Member has already collapsed and must be repositioned or replaced.");
            continue;
        }
        if (!Node.bAnchorCurrent)
        {
            bUnsupported[Index] = true;
            Assessment.Condition = EShoringSupportCondition::NeedsRevalidation;
            Assessment.Reason = TEXT("Support anchor is stale or no longer valid.");
            continue;
        }
        if (Node.AnchorKind == EShoringAnchorKind::PieceSocket)
        {
            const int32* ParentIndex = IndexById.Find(Node.PrimarySupportPieceId);
            if (!ParentIndex
                || !Node.ConnectedPieceIds.Contains(Node.PrimarySupportPieceId)
                || !Nodes[*ParentIndex].ConnectedPieceIds.Contains(Node.PieceId))
            {
                bUnsupported[Index] = true;
                Assessment.Condition = EShoringSupportCondition::Unsupported;
                Assessment.Reason = TEXT("Primary support socket is missing or the connection is not reciprocal.");
                continue;
            }
        }

        for (const FGuid& ConnectedId : Node.ConnectedPieceIds)
        {
            const int32* ConnectedIndex = IndexById.Find(ConnectedId);
            if (!ConnectedIndex || !Nodes[*ConnectedIndex].ConnectedPieceIds.Contains(Node.PieceId))
            {
                bUnsupported[Index] = true;
                Assessment.Condition = EShoringSupportCondition::Unsupported;
                Assessment.Reason = TEXT("A connected piece is missing or its GUID link is not reciprocal.");
                break;
            }
        }
        if (!bUnsupported[Index])
        {
            Assessment.Condition = EShoringSupportCondition::Stable;
        }
    }

    // Every supported piece must have a finite path to a grounded or voxel-anchored
    // root. Cycles of mutually connected pieces cannot support themselves.
    for (int32 StartIndex = 0; StartIndex < Nodes.Num(); ++StartIndex)
    {
        if (bFailed[StartIndex] || bUnsupported[StartIndex])
        {
            continue;
        }

        TSet<int32> Visited;
        int32 Cursor = StartIndex;
        bool bFoundRoot = false;
        for (int32 Step = 0; Step <= Nodes.Num(); ++Step)
        {
            if (bFailed[Cursor] || bUnsupported[Cursor])
            {
                break;
            }
            const FShoringSupportNode& Current = Nodes[Cursor];
            if (Current.AnchorKind != EShoringAnchorKind::PieceSocket)
            {
                bFoundRoot = Current.bAnchorCurrent;
                break;
            }
            if (Visited.Contains(Cursor))
            {
                break;
            }
            Visited.Add(Cursor);
            const int32* ParentIndex = IndexById.Find(Current.PrimarySupportPieceId);
            if (!ParentIndex)
            {
                break;
            }
            Cursor = *ParentIndex;
        }

        if (!bFoundRoot)
        {
            bUnsupported[StartIndex] = true;
            OutAssessments[StartIndex].Condition = EShoringSupportCondition::Unsupported;
            OutAssessments[StartIndex].Reason = TEXT("Support dependency has no valid grounded/voxel root or contains a cycle.");
        }
    }

    // Support loss propagates only along explicit primary-anchor dependencies.
    for (int32 Pass = 0; Pass < Nodes.Num(); ++Pass)
    {
        bool bChanged = false;
        for (int32 Index = 0; Index < Nodes.Num(); ++Index)
        {
            const FShoringSupportNode& Node = Nodes[Index];
            if (bFailed[Index] || bUnsupported[Index] || Node.AnchorKind != EShoringAnchorKind::PieceSocket)
            {
                continue;
            }
            const int32* ParentIndex = IndexById.Find(Node.PrimarySupportPieceId);
            if (!ParentIndex || bFailed[*ParentIndex] || bUnsupported[*ParentIndex])
            {
                bUnsupported[Index] = true;
                OutAssessments[Index].Condition = EShoringSupportCondition::Unsupported;
                OutAssessments[Index].Reason = TEXT("The primary support member failed or lost its own support.");
                bChanged = true;
            }
        }
        if (!bChanged)
        {
            break;
        }
    }

    for (int32 Index = 0; Index < Nodes.Num(); ++Index)
    {
        const FShoringSupportNode& Node = Nodes[Index];
        FShoringSupportAssessment& Assessment = OutAssessments[Index];
        if (bFailed[Index] || bUnsupported[Index])
        {
            continue;
        }

        const float HealthFactor = FMath::Clamp(Node.HealthFraction, MinimumCapacityFactor, 1.0f);
        const float ConnectionFactor = 0.75f + 0.125f * static_cast<float>(FMath::Min(Node.ConnectedPieceIds.Num(), 2));
        Assessment.EffectiveCapacity = Assessment.NominalCapacity * HealthFactor * ConnectionFactor;
        Assessment.Utilization = Assessment.AppliedLoad / FMath::Max(Assessment.EffectiveCapacity, KINDA_SMALL_NUMBER);
        if (!FMath::IsFinite(Assessment.Utilization))
        {
            OutError = TEXT("Support utilization overflowed the finite numeric range.");
            OutAssessments.Reset();
            return false;
        }

        if (Assessment.Utilization >= OverloadUtilization)
        {
            Assessment.Condition = EShoringSupportCondition::Overloaded;
            Assessment.Reason = TEXT("Applied gameplay load exceeds the remaining support capacity.");
        }
        else if (Assessment.Utilization >= WarningUtilization)
        {
            Assessment.Condition = EShoringSupportCondition::Warning;
            Assessment.Reason = TEXT("Support utilization is high; monitor damage and replace weak members.");
        }
        else
        {
            Assessment.Condition = EShoringSupportCondition::Stable;
            Assessment.Reason = TEXT("Support graph and gameplay load are within the configured capacity.");
        }
    }

    return true;
}