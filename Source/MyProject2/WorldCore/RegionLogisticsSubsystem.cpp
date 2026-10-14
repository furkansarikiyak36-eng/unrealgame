#include "RegionLogisticsSubsystem.h"

#include "WorldStateSubsystem.h"
#include "ChunkStateStore.h"
#include "ChunkState.h"
#include "Misc/Crc.h"
#include "Engine/World.h"

namespace
{
    int32 FloorDivide(int32 Value, int32 Divisor)
    {
        const int32 SafeDivisor = FMath::Max(1, Divisor);
        int32 Quotient = Value / SafeDivisor;
        const int32 Remainder = Value % SafeDivisor;
        if (Remainder < 0)
        {
            --Quotient;
        }
        return Quotient;
    }

    bool IsFiniteVector(const FVector& Value)
    {
        return !Value.ContainsNaN()
            && FMath::Abs(Value.X) <= 1000000000.0
            && FMath::Abs(Value.Y) <= 1000000000.0
            && FMath::Abs(Value.Z) <= 1000000000.0;
    }

    bool IsFiniteInRange(float Value, float MinValue, float MaxValue)
    {
        return FMath::IsFinite(Value) && Value >= MinValue && Value <= MaxValue;
    }

    FGuid BuildStableGuid(const TCHAR* Prefix, FName StableKey)
    {
        if (StableKey.IsNone())
        {
            return FGuid();
        }

        const FString Identity = FString(Prefix) + TEXT(":") + StableKey.ToString();
        const uint32 A = FCrc::StrCrc32(*Identity);
        const uint32 B = FCrc::StrCrc32(*Identity, 0x9e3779b9u);
        const uint32 C = FCrc::StrCrc32(*Identity, 0x85ebca6bu);
        uint32 D = FCrc::StrCrc32(*Identity, 0xc2b2ae35u);
        if (A == 0 && B == 0 && C == 0 && D == 0)
        {
            D = 1;
        }
        return FGuid(A, B, C, D);
    }

    float SafeNonNegative(float Value)
    {
        return FMath::IsFinite(Value) ? FMath::Max(0.0f, Value) : 0.0f;
    }
}

void URegionLogisticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ChunksPerRegion = FMath::Clamp(ChunksPerRegion, 1, 128);
}

void URegionLogisticsSubsystem::Deinitialize()
{
    Nodes.Reset();
    Links.Reset();
    OnLogisticsChanged.Clear();
    Super::Deinitialize();
}

FGuid URegionLogisticsSubsystem::MakeStableNodeId(FName StableNodeKey)
{
    return BuildStableGuid(TEXT("REGION_NODE"), StableNodeKey);
}

FGuid URegionLogisticsSubsystem::MakeStableLinkId(FName StableLinkKey)
{
    return BuildStableGuid(TEXT("REGION_LINK"), StableLinkKey);
}

FIntPoint URegionLogisticsSubsystem::GetRegionCoordinateForWorldLocation(const FVector& WorldLocation) const
{
    if (!IsFiniteVector(WorldLocation))
    {
        return FIntPoint::ZeroValue;
    }

    FIntPoint ChunkCoord;
    if (const UWorld* World = GetWorld())
    {
        if (const UWorldStateSubsystem* WorldState = World->GetSubsystem<UWorldStateSubsystem>())
        {
            ChunkCoord = WorldState->ChunkSettings.WorldToChunk(WorldLocation);
        }
        else
        {
            const float ChunkSizeCm = 6400.0f;
            ChunkCoord = FIntPoint(
                FMath::FloorToInt(WorldLocation.X / ChunkSizeCm),
                FMath::FloorToInt(WorldLocation.Y / ChunkSizeCm));
        }
    }
    else
    {
        const float ChunkSizeCm = 6400.0f;
        ChunkCoord = FIntPoint(
            FMath::FloorToInt(WorldLocation.X / ChunkSizeCm),
            FMath::FloorToInt(WorldLocation.Y / ChunkSizeCm));
    }

    const int32 RegionSize = FMath::Clamp(ChunksPerRegion, 1, 128);
    return FIntPoint(FloorDivide(ChunkCoord.X, RegionSize), FloorDivide(ChunkCoord.Y, RegionSize));
}

int32 URegionLogisticsSubsystem::FindNodeIndexById(const FGuid& NodeId) const
{
    return Nodes.IndexOfByPredicate([&NodeId](const FRegionLogisticsNode& Node)
    {
        return Node.NodeId == NodeId;
    });
}

int32 URegionLogisticsSubsystem::FindNodeIndexByKey(FName StableNodeKey) const
{
    return Nodes.IndexOfByPredicate([StableNodeKey](const FRegionLogisticsNode& Node)
    {
        return Node.StableKey == StableNodeKey;
    });
}

int32 URegionLogisticsSubsystem::FindLinkIndexByKey(FName StableLinkKey) const
{
    return Links.IndexOfByPredicate([StableLinkKey](const FRegionLogisticsLink& Link)
    {
        return Link.StableKey == StableLinkKey;
    });
}

bool URegionLogisticsSubsystem::IsValidNodeData(FName StableNodeKey, const FVector& WorldLocation) const
{
    if (StableNodeKey.IsNone() || !IsFiniteVector(WorldLocation))
    {
        return false;
    }

    const FGuid NodeId = MakeStableNodeId(StableNodeKey);
    if (!NodeId.IsValid())
    {
        return false;
    }

    const int32 ExistingIndex = FindNodeIndexById(NodeId);
    return ExistingIndex == INDEX_NONE || Nodes[ExistingIndex].StableKey == StableNodeKey;
}

void URegionLogisticsSubsystem::MarkChanged()
{
    if (Revision < TNumericLimits<int64>::Max())
    {
        ++Revision;
    }
    OnLogisticsChanged.Broadcast(Revision);
}

bool URegionLogisticsSubsystem::RegisterNamedNode(
    FName StableNodeKey,
    const FVector& WorldLocation,
    FName NodeTag,
    bool bEnabled)
{
    if (!IsValidNodeData(StableNodeKey, WorldLocation))
    {
        return false;
    }

    const FGuid NodeId = MakeStableNodeId(StableNodeKey);
    const int32 ExistingIndex = FindNodeIndexById(NodeId);
    if (ExistingIndex != INDEX_NONE)
    {
        FRegionLogisticsNode& Existing = Nodes[ExistingIndex];
        if (Existing.WorldLocation.Equals(WorldLocation, 0.1f)
            && Existing.NodeTag == NodeTag && Existing.bEnabled == bEnabled)
        {
            return true;
        }

        Existing.WorldLocation = WorldLocation;
        Existing.NodeTag = NodeTag;
        Existing.bEnabled = bEnabled;
        MarkChanged();
        return true;
    }

    if (Nodes.Num() >= MaxNodes)
    {
        return false;
    }

    FRegionLogisticsNode NewNode;
    NewNode.NodeId = NodeId;
    NewNode.StableKey = StableNodeKey;
    NewNode.WorldLocation = WorldLocation;
    NewNode.NodeTag = NodeTag;
    NewNode.bEnabled = bEnabled;
    Nodes.Add(MoveTemp(NewNode));
    Nodes.Sort([](const FRegionLogisticsNode& A, const FRegionLogisticsNode& B)
    {
        return A.StableKey.ToString() < B.StableKey.ToString();
    });
    MarkChanged();
    return true;
}

bool URegionLogisticsSubsystem::RemoveNamedNode(FName StableNodeKey)
{
    const int32 NodeIndex = FindNodeIndexByKey(StableNodeKey);
    if (NodeIndex == INDEX_NONE)
    {
        return false;
    }

    const FGuid NodeId = Nodes[NodeIndex].NodeId;
    const bool bIsReferenced = Links.ContainsByPredicate([NodeId](const FRegionLogisticsLink& Link)
    {
        return Link.FromNodeId == NodeId || Link.ToNodeId == NodeId;
    });
    if (bIsReferenced)
    {
        return false;
    }

    Nodes.RemoveAt(NodeIndex);
    MarkChanged();
    return true;
}

bool URegionLogisticsSubsystem::RegisterInfrastructureSegment(
    FName StableLinkKey,
    FName StartNodeKey,
    const FVector& StartLocation,
    FName EndNodeKey,
    const FVector& EndLocation,
    ELogisticsLinkType Type,
    float MaxPayloadKg,
    float Condition01,
    float Risk01,
    bool bBidirectional,
    bool bOperational)
{
    if (StableLinkKey.IsNone() || StartNodeKey.IsNone() || EndNodeKey.IsNone()
        || StartNodeKey == EndNodeKey
        || !IsValidNodeData(StartNodeKey, StartLocation)
        || !IsValidNodeData(EndNodeKey, EndLocation)
        || FVector::Distance(StartLocation, EndLocation) < 1.0f
        || !IsFiniteInRange(MaxPayloadKg, 0.1f, 10000000.0f)
        || !IsFiniteInRange(Condition01, 0.0f, 1.0f)
        || !IsFiniteInRange(Risk01, 0.0f, 1.0f)
        || static_cast<uint8>(Type) > static_cast<uint8>(ELogisticsLinkType::Ropeway))
    {
        return false;
    }

    const FGuid LinkId = MakeStableLinkId(StableLinkKey);
    if (!LinkId.IsValid())
    {
        return false;
    }
    const int32 ExistingLinkIndex = FindLinkIndexByKey(StableLinkKey);
    const int32 ExistingById = Links.IndexOfByPredicate([LinkId](const FRegionLogisticsLink& Link)
    {
        return Link.LinkId == LinkId;
    });
    if (ExistingById != INDEX_NONE && Links[ExistingById].StableKey != StableLinkKey)
    {
        return false;
    }

    const bool bNeedsStartNode = FindNodeIndexByKey(StartNodeKey) == INDEX_NONE;
    const bool bNeedsEndNode = FindNodeIndexByKey(EndNodeKey) == INDEX_NONE;
    if (Nodes.Num() + static_cast<int32>(bNeedsStartNode) + static_cast<int32>(bNeedsEndNode) > MaxNodes
        || (ExistingLinkIndex == INDEX_NONE && Links.Num() >= MaxLinks))
    {
        return false;
    }

    // All inputs and capacity limits are checked before the graph is mutated.
    if (!RegisterNamedNode(StartNodeKey, StartLocation, StartNodeKey, true)
        || !RegisterNamedNode(EndNodeKey, EndLocation, EndNodeKey, true))
    {
        return false;
    }

    FRegionLogisticsLink NewLink;
    NewLink.LinkId = LinkId;
    NewLink.StableKey = StableLinkKey;
    NewLink.FromNodeId = MakeStableNodeId(StartNodeKey);
    NewLink.ToNodeId = MakeStableNodeId(EndNodeKey);
    NewLink.Type = Type;
    NewLink.MaxPayloadKg = MaxPayloadKg;
    NewLink.Condition01 = Condition01;
    NewLink.Risk01 = Risk01;
    NewLink.bBidirectional = bBidirectional;
    NewLink.bOperational = bOperational;

    if (ExistingLinkIndex != INDEX_NONE)
    {
        Links[ExistingLinkIndex] = MoveTemp(NewLink);
    }
    else
    {
        Links.Add(MoveTemp(NewLink));
    }
    Links.Sort([](const FRegionLogisticsLink& A, const FRegionLogisticsLink& B)
    {
        return A.StableKey.ToString() < B.StableKey.ToString();
    });
    MarkChanged();
    return true;
}

bool URegionLogisticsSubsystem::SetInfrastructureStatus(
    FName StableLinkKey,
    bool bOperational,
    float Condition01,
    float Risk01)
{
    if (!IsFiniteInRange(Condition01, 0.0f, 1.0f) || !IsFiniteInRange(Risk01, 0.0f, 1.0f))
    {
        return false;
    }

    const int32 LinkIndex = FindLinkIndexByKey(StableLinkKey);
    if (LinkIndex == INDEX_NONE)
    {
        return false;
    }

    FRegionLogisticsLink& Link = Links[LinkIndex];
    if (Link.bOperational == bOperational && FMath::IsNearlyEqual(Link.Condition01, Condition01)
        && FMath::IsNearlyEqual(Link.Risk01, Risk01))
    {
        return true;
    }

    Link.bOperational = bOperational;
    Link.Condition01 = Condition01;
    Link.Risk01 = Risk01;
    MarkChanged();
    return true;
}

void URegionLogisticsSubsystem::RemoveOrphanNode(const FGuid& NodeId)
{
    const bool bHasLinks = Links.ContainsByPredicate([NodeId](const FRegionLogisticsLink& Link)
    {
        return Link.FromNodeId == NodeId || Link.ToNodeId == NodeId;
    });
    if (bHasLinks)
    {
        return;
    }

    const int32 NodeIndex = FindNodeIndexById(NodeId);
    if (NodeIndex != INDEX_NONE)
    {
        Nodes.RemoveAt(NodeIndex);
    }
}

bool URegionLogisticsSubsystem::UnregisterInfrastructureSegment(FName StableLinkKey)
{
    const int32 LinkIndex = FindLinkIndexByKey(StableLinkKey);
    if (LinkIndex == INDEX_NONE)
    {
        return false;
    }

    const FGuid FromNodeId = Links[LinkIndex].FromNodeId;
    const FGuid ToNodeId = Links[LinkIndex].ToNodeId;
    Links.RemoveAt(LinkIndex);
    RemoveOrphanNode(FromNodeId);
    RemoveOrphanNode(ToNodeId);
    MarkChanged();
    return true;
}

float URegionLogisticsSubsystem::GetLinkBaseSpeedMetersPerSecond(ELogisticsLinkType Type) const
{
    switch (Type)
    {
    case ELogisticsLinkType::Trail: return 1.1f;
    case ELogisticsLinkType::Road: return 2.7f;
    case ELogisticsLinkType::Bridge: return 2.1f;
    case ELogisticsLinkType::Ramp: return 0.9f;
    case ELogisticsLinkType::SledRoute: return 1.6f;
    case ELogisticsLinkType::Ropeway: return 4.0f;
    default: return 0.0f;
    }
}

float URegionLogisticsSubsystem::GetEnvironmentalRiskCached(
    const FVector& Location,
    TMap<FIntPoint, float>& RiskCache) const
{
    const FIntPoint RegionCoord = GetRegionCoordinateForWorldLocation(Location);
    if (const float* CachedRisk = RiskCache.Find(RegionCoord))
    {
        return *CachedRisk;
    }

    FRegionSurvey Survey;
    const float Risk = QueryRegionSurvey(RegionCoord, Survey)
        ? FMath::Clamp(Survey.EnvironmentalRisk01, 0.0f, 1.0f)
        : 0.0f;
    RiskCache.Add(RegionCoord, Risk);
    return Risk;
}

bool URegionLogisticsSubsystem::ComputeTraversalCost(
    const FRegionLogisticsLink& Link,
    const FRegionLogisticsNode& FromNode,
    const FRegionLogisticsNode& ToNode,
    float PayloadMassKg,
    TMap<FIntPoint, float>& RiskCache,
    float& OutDistanceMeters,
    float& OutSeconds,
    float& OutRisk01) const
{
    OutDistanceMeters = 0.0f;
    OutSeconds = 0.0f;
    OutRisk01 = 0.0f;

    if (!Link.bOperational || !FromNode.bEnabled || !ToNode.bEnabled
        || !FMath::IsFinite(PayloadMassKg) || PayloadMassKg < 0.0f
        || (PayloadMassKg > 0.0f && PayloadMassKg > Link.MaxPayloadKg))
    {
        return false;
    }

    const float Speed = GetLinkBaseSpeedMetersPerSecond(Link.Type);
    if (!FMath::IsFinite(Speed) || Speed <= 0.0f)
    {
        return false;
    }

    const float DistanceMeters = FVector::Distance(FromNode.WorldLocation, ToNode.WorldLocation) / 100.0f;
    if (!FMath::IsFinite(DistanceMeters) || DistanceMeters <= 0.0f)
    {
        return false;
    }

    const FVector Midpoint = (FromNode.WorldLocation + ToNode.WorldLocation) * 0.5f;
    const float RegionalRisk = GetEnvironmentalRiskCached(Midpoint, RiskCache);
    const float EffectiveRisk = FMath::Clamp(Link.Risk01 + RegionalRisk * 0.5f, 0.0f, 1.0f);
    const float ConditionPenalty = 1.0f + (1.0f - Link.Condition01) * 1.5f;
    const float RiskPenalty = 1.0f + EffectiveRisk * 2.0f;

    OutDistanceMeters = DistanceMeters;
    OutRisk01 = EffectiveRisk;
    OutSeconds = (DistanceMeters / Speed) * ConditionPenalty * RiskPenalty;
    return FMath::IsFinite(OutSeconds) && OutSeconds > 0.0f;
}

bool URegionLogisticsSubsystem::FindLowestCostRouteByKey(
    FName StartNodeKey,
    FName DestinationNodeKey,
    float PayloadMassKg,
    FRegionRouteResult& OutRoute) const
{
    OutRoute = FRegionRouteResult();
    if (StartNodeKey.IsNone() || DestinationNodeKey.IsNone()
        || !FMath::IsFinite(PayloadMassKg) || PayloadMassKg < 0.0f)
    {
        return false;
    }
    return FindLowestCostRoute(MakeStableNodeId(StartNodeKey), MakeStableNodeId(DestinationNodeKey), PayloadMassKg, OutRoute);
}

bool URegionLogisticsSubsystem::FindLowestCostRoute(
    const FGuid& StartNodeId,
    const FGuid& DestinationNodeId,
    float PayloadMassKg,
    FRegionRouteResult& OutRoute) const
{
    OutRoute = FRegionRouteResult();
    const int32 StartIndex = FindNodeIndexById(StartNodeId);
    const int32 DestinationIndex = FindNodeIndexById(DestinationNodeId);
    if (StartIndex == INDEX_NONE || DestinationIndex == INDEX_NONE
        || !Nodes[StartIndex].bEnabled || !Nodes[DestinationIndex].bEnabled
        || !FMath::IsFinite(PayloadMassKg) || PayloadMassKg < 0.0f)
    {
        return false;
    }

    if (StartIndex == DestinationIndex)
    {
        OutRoute.bFound = true;
        OutRoute.NodeIds.Add(StartNodeId);
        return true;
    }

    TMap<FGuid, int32> NodeIndexById;
    TMap<FGuid, TArray<int32>> Adjacency;
    NodeIndexById.Reserve(Nodes.Num());
    Adjacency.Reserve(Nodes.Num());
    for (int32 Index = 0; Index < Nodes.Num(); ++Index)
    {
        NodeIndexById.Add(Nodes[Index].NodeId, Index);
        Adjacency.FindOrAdd(Nodes[Index].NodeId);
    }
    for (int32 LinkIndex = 0; LinkIndex < Links.Num(); ++LinkIndex)
    {
        const FRegionLogisticsLink& Link = Links[LinkIndex];
        Adjacency.FindOrAdd(Link.FromNodeId).Add(LinkIndex);
        if (Link.bBidirectional)
        {
            Adjacency.FindOrAdd(Link.ToNodeId).Add(LinkIndex);
        }
    }
    for (TPair<FGuid, TArray<int32>>& Pair : Adjacency)
    {
        Pair.Value.Sort([this](int32 A, int32 B)
        {
            return Links[A].StableKey.ToString() < Links[B].StableKey.ToString();
        });
    }

    const float Infinity = TNumericLimits<float>::Max();
    TArray<float> BestSeconds;
    TArray<int32> PreviousNode;
    TArray<int32> PreviousLink;
    TArray<uint8> Visited;
    BestSeconds.Init(Infinity, Nodes.Num());
    PreviousNode.Init(INDEX_NONE, Nodes.Num());
    PreviousLink.Init(INDEX_NONE, Nodes.Num());
    Visited.Init(0, Nodes.Num());
    BestSeconds[StartIndex] = 0.0f;

    TMap<FIntPoint, float> RegionalRiskCache;
    bool bReachedDestination = false;
    for (int32 Iteration = 0; Iteration < Nodes.Num(); ++Iteration)
    {
        int32 CurrentIndex = INDEX_NONE;
        float CurrentCost = Infinity;
        for (int32 Index = 0; Index < Nodes.Num(); ++Index)
        {
            if (Visited[Index] || BestSeconds[Index] >= Infinity)
            {
                continue;
            }
            if (CurrentIndex == INDEX_NONE || BestSeconds[Index] < CurrentCost - KINDA_SMALL_NUMBER
                || (FMath::IsNearlyEqual(BestSeconds[Index], CurrentCost)
                    && Nodes[Index].StableKey.ToString() < Nodes[CurrentIndex].StableKey.ToString()))
            {
                CurrentIndex = Index;
                CurrentCost = BestSeconds[Index];
            }
        }

        if (CurrentIndex == INDEX_NONE)
        {
            break;
        }
        if (CurrentIndex == DestinationIndex)
        {
            bReachedDestination = true;
            break;
        }
        Visited[CurrentIndex] = 1;

        const FGuid CurrentNodeId = Nodes[CurrentIndex].NodeId;
        const TArray<int32>* CurrentLinks = Adjacency.Find(CurrentNodeId);
        if (!CurrentLinks)
        {
            continue;
        }

        for (const int32 LinkIndex : *CurrentLinks)
        {
            const FRegionLogisticsLink& Link = Links[LinkIndex];
            FGuid OtherNodeId;
            if (Link.FromNodeId == CurrentNodeId)
            {
                OtherNodeId = Link.ToNodeId;
            }
            else if (Link.bBidirectional && Link.ToNodeId == CurrentNodeId)
            {
                OtherNodeId = Link.FromNodeId;
            }
            else
            {
                continue;
            }

            const int32* OtherIndexPtr = NodeIndexById.Find(OtherNodeId);
            if (!OtherIndexPtr || Visited[*OtherIndexPtr])
            {
                continue;
            }
            const int32 OtherIndex = *OtherIndexPtr;

            float DistanceMeters = 0.0f;
            float EdgeSeconds = 0.0f;
            float EdgeRisk = 0.0f;
            if (!ComputeTraversalCost(Link, Nodes[CurrentIndex], Nodes[OtherIndex], PayloadMassKg,
                    RegionalRiskCache, DistanceMeters, EdgeSeconds, EdgeRisk))
            {
                continue;
            }

            const float Candidate = BestSeconds[CurrentIndex] + EdgeSeconds;
            const bool bStrictlyBetter = Candidate < BestSeconds[OtherIndex] - KINDA_SMALL_NUMBER;
            const bool bStableTieBetter = FMath::IsNearlyEqual(Candidate, BestSeconds[OtherIndex])
                && (PreviousNode[OtherIndex] == INDEX_NONE
                    || Nodes[CurrentIndex].StableKey.ToString() < Nodes[PreviousNode[OtherIndex]].StableKey.ToString());
            if (bStrictlyBetter || bStableTieBetter)
            {
                BestSeconds[OtherIndex] = Candidate;
                PreviousNode[OtherIndex] = CurrentIndex;
                PreviousLink[OtherIndex] = LinkIndex;
            }
        }
    }

    if (!bReachedDestination)
    {
        return false;
    }

    TArray<int32> ReverseNodeIndices;
    TArray<int32> ReverseLinkIndices;
    int32 Cursor = DestinationIndex;
    ReverseNodeIndices.Add(Cursor);
    while (Cursor != StartIndex && ReverseNodeIndices.Num() <= Nodes.Num())
    {
        const int32 Previous = PreviousNode[Cursor];
        const int32 LinkIndex = PreviousLink[Cursor];
        if (Previous == INDEX_NONE || LinkIndex == INDEX_NONE)
        {
            OutRoute = FRegionRouteResult();
            return false;
        }
        ReverseLinkIndices.Add(LinkIndex);
        Cursor = Previous;
        ReverseNodeIndices.Add(Cursor);
    }
    if (Cursor != StartIndex)
    {
        OutRoute = FRegionRouteResult();
        return false;
    }

    ReverseNodeIndices.Reverse();
    ReverseLinkIndices.Reverse();
    OutRoute.bFound = true;
    OutRoute.EstimatedTravelSeconds = BestSeconds[DestinationIndex];
    OutRoute.BottleneckCapacityKg = TNumericLimits<float>::Max();

    for (const int32 NodeIndex : ReverseNodeIndices)
    {
        OutRoute.NodeIds.Add(Nodes[NodeIndex].NodeId);
    }
    for (int32 PathIndex = 0; PathIndex < ReverseLinkIndices.Num(); ++PathIndex)
    {
        const int32 LinkIndex = ReverseLinkIndices[PathIndex];
        const FRegionLogisticsLink& Link = Links[LinkIndex];
        const int32 FromIndex = ReverseNodeIndices[PathIndex];
        const int32 ToIndex = ReverseNodeIndices[PathIndex + 1];

        float DistanceMeters = 0.0f;
        float EdgeSeconds = 0.0f;
        float EdgeRisk = 0.0f;
        if (!ComputeTraversalCost(Link, Nodes[FromIndex], Nodes[ToIndex], PayloadMassKg,
                RegionalRiskCache, DistanceMeters, EdgeSeconds, EdgeRisk))
        {
            OutRoute = FRegionRouteResult();
            return false;
        }

        OutRoute.LinkIds.Add(Link.LinkId);
        OutRoute.TotalDistanceMeters += DistanceMeters;
        OutRoute.MaximumRisk01 = FMath::Max(OutRoute.MaximumRisk01, EdgeRisk);
        OutRoute.BottleneckCapacityKg = FMath::Min(OutRoute.BottleneckCapacityKg, Link.MaxPayloadKg);
    }
    if (OutRoute.LinkIds.Num() == 0)
    {
        OutRoute.BottleneckCapacityKg = 0.0f;
    }
    return true;
}

bool URegionLogisticsSubsystem::QueryRegionSurvey(const FIntPoint& RegionCoord, FRegionSurvey& OutSurvey) const
{
    OutSurvey = FRegionSurvey();
    OutSurvey.RegionCoord = RegionCoord;

    const UWorld* World = GetWorld();
    const UWorldStateSubsystem* WorldState = World ? World->GetSubsystem<UWorldStateSubsystem>() : nullptr;
    if (!WorldState || !WorldState->StateStore)
    {
        return false;
    }

    double RiskTotal = 0.0;
    double BiomassTotal = 0.0;
    double NutrientTotal = 0.0;
    double WaterTotal = 0.0;
    for (const UChunkState* Chunk : WorldState->StateStore->GetAllChunks())
    {
        if (!Chunk || GetRegionCoordinateForWorldLocation(
                WorldState->ChunkSettings.ChunkToWorldCenter(Chunk->Coordinate)) != RegionCoord)
        {
            continue;
        }

        ++OutSurvey.ChunksSurveyed;
        for (const FChunkCell& Cell : Chunk->Cells)
        {
            ++OutSurvey.CellsSurveyed;
            BiomassTotal += SafeNonNegative(Cell.Biomass.Biomass);
            NutrientTotal += SafeNonNegative(Cell.SoilNitrogen)
                + SafeNonNegative(Cell.SoilPhosphorus) + SafeNonNegative(Cell.SoilPotassium);
            WaterTotal += SafeNonNegative(Cell.Water.Volume);

            float CellRisk = Cell.bHasFire || Cell.Biomass.bIsBurning ? 0.65f : 0.0f;
            CellRisk += FMath::Clamp(SafeNonNegative(Cell.Chemistry.ToxinConcentration) / 100.0f, 0.0f, 1.0f) * 0.25f;
            if (Cell.bHasWater || Cell.Water.Volume > 0.0f)
            {
                CellRisk += 0.10f;
            }
            RiskTotal += FMath::Clamp(CellRisk, 0.0f, 1.0f);
        }
    }

    if (OutSurvey.CellsSurveyed <= 0)
    {
        return false;
    }

    OutSurvey.AvailableBiomassEstimate = static_cast<float>(FMath::Min<double>(BiomassTotal, TNumericLimits<float>::Max()));
    OutSurvey.SoilNutrientEstimate = static_cast<float>(FMath::Min<double>(NutrientTotal, TNumericLimits<float>::Max()));
    OutSurvey.StandingWaterVolume = static_cast<float>(FMath::Min<double>(WaterTotal, TNumericLimits<float>::Max()));
    OutSurvey.EnvironmentalRisk01 = FMath::Clamp(
        static_cast<float>(RiskTotal / static_cast<double>(OutSurvey.CellsSurveyed)), 0.0f, 1.0f);
    return true;
}

bool URegionLogisticsSubsystem::ExportSnapshot(FRegionLogisticsSnapshot& OutSnapshot) const
{
    FRegionLogisticsSnapshot Snapshot;
    Snapshot.SchemaVersion = 1;
    Snapshot.Revision = Revision;
    Snapshot.Nodes = Nodes;
    Snapshot.Links = Links;
    if (!ValidateSnapshot(Snapshot))
    {
        return false;
    }
    OutSnapshot = MoveTemp(Snapshot);
    return true;
}

bool URegionLogisticsSubsystem::ValidateSnapshot(const FRegionLogisticsSnapshot& Snapshot) const
{
    if (Snapshot.SchemaVersion != 1 || Snapshot.Revision < 0
        || Snapshot.Nodes.Num() > MaxNodes || Snapshot.Links.Num() > MaxLinks)
    {
        return false;
    }

    TSet<FGuid> SeenNodeIds;
    TSet<FName> SeenNodeKeys;
    SeenNodeIds.Reserve(Snapshot.Nodes.Num());
    SeenNodeKeys.Reserve(Snapshot.Nodes.Num());
    for (const FRegionLogisticsNode& Node : Snapshot.Nodes)
    {
        if (!Node.NodeId.IsValid() || Node.StableKey.IsNone()
            || Node.NodeId != MakeStableNodeId(Node.StableKey)
            || !IsFiniteVector(Node.WorldLocation)
            || SeenNodeIds.Contains(Node.NodeId) || SeenNodeKeys.Contains(Node.StableKey))
        {
            return false;
        }
        SeenNodeIds.Add(Node.NodeId);
        SeenNodeKeys.Add(Node.StableKey);
    }

    TSet<FGuid> SeenLinkIds;
    TSet<FName> SeenLinkKeys;
    SeenLinkIds.Reserve(Snapshot.Links.Num());
    SeenLinkKeys.Reserve(Snapshot.Links.Num());
    for (const FRegionLogisticsLink& Link : Snapshot.Links)
    {
        if (!Link.LinkId.IsValid() || Link.StableKey.IsNone()
            || Link.LinkId != MakeStableLinkId(Link.StableKey)
            || Link.FromNodeId == Link.ToNodeId
            || !SeenNodeIds.Contains(Link.FromNodeId) || !SeenNodeIds.Contains(Link.ToNodeId)
            || static_cast<uint8>(Link.Type) > static_cast<uint8>(ELogisticsLinkType::Ropeway)
            || !IsFiniteInRange(Link.MaxPayloadKg, 0.1f, 10000000.0f)
            || !IsFiniteInRange(Link.Condition01, 0.0f, 1.0f)
            || !IsFiniteInRange(Link.Risk01, 0.0f, 1.0f)
            || SeenLinkIds.Contains(Link.LinkId) || SeenLinkKeys.Contains(Link.StableKey))
        {
            return false;
        }
        SeenLinkIds.Add(Link.LinkId);
        SeenLinkKeys.Add(Link.StableKey);
    }
    return true;
}

bool URegionLogisticsSubsystem::ImportSnapshot(const FRegionLogisticsSnapshot& Snapshot)
{
    if (!ValidateSnapshot(Snapshot))
    {
        return false;
    }

    TArray<FRegionLogisticsNode> NewNodes = Snapshot.Nodes;
    TArray<FRegionLogisticsLink> NewLinks = Snapshot.Links;
    NewNodes.Sort([](const FRegionLogisticsNode& A, const FRegionLogisticsNode& B)
    {
        return A.StableKey.ToString() < B.StableKey.ToString();
    });
    NewLinks.Sort([](const FRegionLogisticsLink& A, const FRegionLogisticsLink& B)
    {
        return A.StableKey.ToString() < B.StableKey.ToString();
    });

    Nodes = MoveTemp(NewNodes);
    Links = MoveTemp(NewLinks);
    Revision = Snapshot.Revision;
    OnLogisticsChanged.Broadcast(Revision);
    return true;
}
