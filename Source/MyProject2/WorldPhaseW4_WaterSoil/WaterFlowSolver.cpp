#include "WaterFlowSolver.h"

void UWaterFlowSolver::SolveFlow(UWaterLayer* Layer, float DeltaTime)
{
    if (!Layer || !Layer->bIsActive || DeltaTime <= 0.0f) return;
    const int32 Axis = FMath::Max(1, Layer->CellsPerAxis);
    TMap<int32, float> Delta;
    TMap<int32, float> Outgoing;
    TMap<int32, float> Rates;

    for (const auto& Pair : Layer->Cells)
    {
        const int32 Index = Pair.Key;
        if (Index < 0 || Index >= Axis * Axis) continue;
        const int32 X = Index % Axis;
        const int32 Y = Index / Axis;
        const int32 Neighbors[2] = { X + 1 < Axis ? Index + 1 : INDEX_NONE, Y + 1 < Axis ? Index + Axis : INDEX_NONE };
        for (const int32 NeighborIndex : Neighbors)
        {
            if (NeighborIndex == INDEX_NONE) continue;
            const FWaterCell* A = Layer->Cells.Find(Index);
            const FWaterCell* B = Layer->Cells.Find(NeighborIndex);
            if (!A || !B) continue;
            const float Difference = A->Level - B->Level;
            if (FMath::Abs(Difference) <= MinFlowThreshold) continue;

            const int32 FromIndex = Difference > 0.0f ? Index : NeighborIndex;
            const int32 ToIndex = Difference > 0.0f ? NeighborIndex : Index;
            const FWaterCell* FromCell = Layer->Cells.Find(FromIndex);
            if (!FromCell) continue;
            const float Cap = FMath::Max(0.0f, FromCell->Volume * FMath::Clamp(MaxTransferFraction, 0.0f, 1.0f));
            const float Available = FMath::Max(0.0f, FMath::Min(FromCell->Volume, Cap) - Outgoing.FindRef(FromIndex));
            const float Proposed = FMath::Max(0.0f, FMath::Abs(Difference) * FMath::Max(0.0f, Gravity) * DeltaTime / (1.0f + FMath::Max(0.0f, FlowResistance)));
            const float Transfer = FMath::Min(Available, Proposed);
            if (Transfer <= MinFlowThreshold) continue;
            Delta.FindOrAdd(FromIndex) -= Transfer;
            Delta.FindOrAdd(ToIndex) += Transfer;
            Outgoing.FindOrAdd(FromIndex) += Transfer;
            Rates.Add(FromIndex, Outgoing.FindRef(FromIndex) / DeltaTime);
            Rates.Add(ToIndex, -Transfer / DeltaTime);
        }
    }

    for (auto& Pair : Layer->Cells)
    {
        Pair.Value.Volume = FMath::Max(0.0f, Pair.Value.Volume + Delta.FindRef(Pair.Key));
        Pair.Value.Level = Pair.Value.Volume;
        Pair.Value.FlowRate = Rates.FindRef(Pair.Key);
        Pair.Value.bIsFlowing = FMath::Abs(Pair.Value.FlowRate) > MinFlowThreshold;
    }
    Layer->RecomputeTotals();
}

void UWaterFlowSolver::FlowBetweenCells(FWaterCell& From, FWaterCell& To, float HeightDiff, float DeltaTime)
{
    if (HeightDiff <= 0.0f || DeltaTime <= 0.0f) return;
    const float Flow = HeightDiff * FMath::Max(0.0f, Gravity) * DeltaTime / (1.0f + FMath::Max(0.0f, FlowResistance));
    const float Transfer = FMath::Min(From.Volume * FMath::Clamp(MaxTransferFraction, 0.0f, 1.0f), Flow);
    From.Volume -= Transfer; To.Volume += Transfer;
    From.FlowRate = Transfer / DeltaTime; To.FlowRate = -Transfer / DeltaTime;
}
