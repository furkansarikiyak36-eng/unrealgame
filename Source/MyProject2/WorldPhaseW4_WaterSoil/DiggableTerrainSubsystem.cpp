#include "DiggableTerrainSubsystem.h"

#include "WorldCore/WorldStateSubsystem.h"
#include "WorldCore/ChunkState.h"
#include "Engine/World.h"

void UDiggableTerrainSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UWorldStateSubsystem>();
    WorldState = GetWorld() ? GetWorld()->GetSubsystem<UWorldStateSubsystem>() : nullptr;
}

void UDiggableTerrainSubsystem::Deinitialize()
{
    WorldState = nullptr;
    Super::Deinitialize();
}

bool UDiggableTerrainSubsystem::ResolveExistingCell(const FVector& P, UChunkState*& OutChunk, FChunkCell*& OutCell, FIntPoint& OutChunkCoord) const
{
    OutChunk = nullptr;
    OutCell = nullptr;
    OutChunkCoord = FIntPoint::ZeroValue;
    UWorldStateSubsystem* State = WorldState.Get();
    if (!State) return false;
    OutChunkCoord = State->ChunkSettings.WorldToChunk(P);
    OutChunk = State->GetChunk(OutChunkCoord);
    if (!OutChunk) return false;
    const FIntPoint Local = State->ChunkSettings.WorldToLocalCell(P);
    OutCell = OutChunk->GetCell(Local.X, Local.Y, State->ChunkSettings.CellsPerChunkAxis);
    return OutCell != nullptr;
}

void UDiggableTerrainSubsystem::GatherCandidates(const FVector& P, float RadiusMeters, EDigBrushShape Shape, TArray<FCellCandidate>& Out) const
{
    Out.Reset();
    const UWorldStateSubsystem* State = WorldState.Get();
    if (!State) return;
    const float CellCm = FMath::Max(1.0f, State->ChunkSettings.GetCellSizeMeters() * 100.0f);
    const float RadiusCm = Shape == EDigBrushShape::GridCell ? 0.0f
        : FMath::Clamp(RadiusMeters, CellCm / 200.0f, MaximumBrushRadiusMeters) * 100.0f;
    const int32 CenterX = FMath::FloorToInt(P.X / CellCm);
    const int32 CenterY = FMath::FloorToInt(P.Y / CellCm);
    const int32 MinX = Shape == EDigBrushShape::GridCell ? CenterX : FMath::FloorToInt((P.X - RadiusCm - CellCm * 0.7072f) / CellCm);
    const int32 MaxX = Shape == EDigBrushShape::GridCell ? CenterX : FMath::FloorToInt((P.X + RadiusCm + CellCm * 0.7072f) / CellCm);
    const int32 MinY = Shape == EDigBrushShape::GridCell ? CenterY : FMath::FloorToInt((P.Y - RadiusCm - CellCm * 0.7072f) / CellCm);
    const int32 MaxY = Shape == EDigBrushShape::GridCell ? CenterY : FMath::FloorToInt((P.Y + RadiusCm + CellCm * 0.7072f) / CellCm);
    const float CellHalfDiagonal = CellCm * 0.7072f;

    for (int32 GX = MinX; GX <= MaxX; ++GX)
    {
        for (int32 GY = MinY; GY <= MaxY; ++GY)
        {
            const FVector Center((GX + 0.5f) * CellCm, (GY + 0.5f) * CellCm, P.Z);
            float Weight = 1.0f;
            if (Shape != EDigBrushShape::GridCell)
            {
                const float DX = FMath::Abs(Center.X - P.X);
                const float DY = FMath::Abs(Center.Y - P.Y);
                const float Distance = Shape == EDigBrushShape::Square ? FMath::Max(DX, DY) : FVector2D(DX, DY).Length();
                const float EdgeDistance = FMath::Max(0.0f, Distance - CellHalfDiagonal);
                if (EdgeDistance > RadiusCm) continue;
                const float T = RadiusCm > KINDA_SMALL_NUMBER ? FMath::Clamp(EdgeDistance / RadiusCm, 0.0f, 1.0f) : 0.0f;
                Weight = 1.0f - (T * T * (3.0f - 2.0f * T));
                Weight = FMath::Max(0.05f, Weight);
            }
            FCellCandidate& C = Out.AddDefaulted_GetRef();
            C.WorldCenter = Center;
            C.Weight = Weight;
        }
    }
}

void UDiggableTerrainSubsystem::BroadcastChange(const TSet<FIntPoint>& ChangedChunks, int32 ChangedCells, float ExcavatedM3, float FilledM3)
{
    if (ChangedCells <= 0) return;
    TArray<FIntPoint> Coords = ChangedChunks.Array();
    Coords.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X == B.X ? A.Y < B.Y : A.X < B.X; });
    OnTerrainChanged.Broadcast(Coords, ChangedCells, ExcavatedM3, FilledM3);
}

FDigOperationResult UDiggableTerrainSubsystem::DigAtWorld(const FVector& WorldLocation, float RadiusMeters, float DepthMeters, EDigBrushShape Shape)
{
    FDigOperationResult Result;
    UWorldStateSubsystem* State = WorldState.Get();
    if (!State) { Result.FailureReason = FText::FromString(TEXT("World state subsystem is unavailable.")); return Result; }
    if (DepthMeters <= 0.0f) { Result.FailureReason = FText::FromString(TEXT("Dig depth must be greater than zero.")); return Result; }

    TArray<FCellCandidate> Candidates;
    GatherCandidates(WorldLocation, RadiusMeters, Shape, Candidates);
    if (Candidates.IsEmpty()) { Result.FailureReason = FText::FromString(TEXT("Brush does not overlap any terrain cells.")); return Result; }

    const float CellArea = FMath::Square(State->ChunkSettings.GetCellSizeMeters());
    TSet<FIntPoint> ChangedChunks;
    for (const FCellCandidate& Candidate : Candidates)
    {
        const FIntPoint ChunkCoord = State->ChunkSettings.WorldToChunk(Candidate.WorldCenter);
        UChunkState* Chunk = State->GetOrCreateChunkAtWorld(Candidate.WorldCenter);
        if (!Chunk) continue;
        const FIntPoint Local = State->ChunkSettings.WorldToLocalCell(Candidate.WorldCenter);
        FChunkCell* Cell = Chunk->GetCell(Local.X, Local.Y, State->ChunkSettings.CellsPerChunkAxis);
        if (!Cell || Cell->Terrain.SoilTexture == EWorldSoilTexture::Rock) continue;

        const float SoilRemaining = FMath::Max(0.0f, Cell->Terrain.SoilDepthMeters - MinimumSoilDepthMeters);
        const float BedrockRemaining = FMath::Max(0.0f, Cell->Terrain.BedrockDepthMeters - Cell->Terrain.ExcavatedDepthMeters);
        const float MaxCut = FMath::Min(SoilRemaining, BedrockRemaining);
        const float CutDepth = FMath::Min(FMath::Min(DepthMeters, MaximumDigDepthPerActionMeters) * Candidate.Weight, MaxCut);
        if (CutDepth <= KINDA_SMALL_NUMBER) continue;

        Cell->Terrain.Height -= CutDepth;
        Cell->Terrain.Deformation -= CutDepth;
        Cell->Terrain.SoilDepthMeters -= CutDepth;
        Cell->Terrain.ExcavatedDepthMeters += CutDepth;
        Cell->Terrain.Compaction = FMath::Clamp(Cell->Terrain.Compaction - 0.12f, 0.0f, 1.0f);
        Cell->Terrain.RootingDepthMeters = FMath::Min(Cell->Terrain.RootingDepthMeters, FMath::Max(MinimumSoilDepthMeters, Cell->Terrain.SoilDepthMeters));
        Cell->Terrain.GroundTreatment = EWorldGroundTreatment::Disturbed;
        Cell->bIsDirty = true;
        Chunk->MarkDirty();
        ++Result.CellsChanged;
        Result.ExcavatedVolumeM3 += CutDepth * CellArea;
        ChangedChunks.Add(ChunkCoord);
    }

    Result.bSucceeded = Result.CellsChanged > 0;
    if (!Result.bSucceeded) Result.FailureReason = FText::FromString(TEXT("No diggable soil was found; the area may be bedrock or at its depth limit."));
    Result.AffectedChunks = ChangedChunks.Array();
    Result.AffectedChunks.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X == B.X ? A.Y < B.Y : A.X < B.X; });
    BroadcastChange(ChangedChunks, Result.CellsChanged, Result.ExcavatedVolumeM3, 0.0f);
    return Result;
}

FDigOperationResult UDiggableTerrainSubsystem::FillAtWorld(const FVector& WorldLocation, float RadiusMeters, float AvailableLooseSoilM3, EDigBrushShape Shape)
{
    FDigOperationResult Result;
    UWorldStateSubsystem* State = WorldState.Get();
    if (!State) { Result.FailureReason = FText::FromString(TEXT("World state subsystem is unavailable.")); return Result; }
    if (AvailableLooseSoilM3 <= 0.0f) { Result.FailureReason = FText::FromString(TEXT("No loose soil is available for filling.")); return Result; }

    TArray<FCellCandidate> Candidates;
    GatherCandidates(WorldLocation, RadiusMeters, Shape, Candidates);
    if (Candidates.IsEmpty()) { Result.FailureReason = FText::FromString(TEXT("Brush does not overlap any terrain cells.")); return Result; }
    const float CellArea = FMath::Square(State->ChunkSettings.GetCellSizeMeters());
    float LooseRemaining = AvailableLooseSoilM3;
    TSet<FIntPoint> ChangedChunks;

    for (const FCellCandidate& Candidate : Candidates)
    {
        if (LooseRemaining <= KINDA_SMALL_NUMBER) break;
        UChunkState* Chunk = nullptr;
        FChunkCell* Cell = nullptr;
        FIntPoint ChunkCoord;
        if (!ResolveExistingCell(Candidate.WorldCenter, Chunk, Cell, ChunkCoord) || !Cell) continue;
        if (Cell->Terrain.SoilTexture == EWorldSoilTexture::Rock || Cell->Terrain.ExcavatedDepthMeters <= KINDA_SMALL_NUMBER) continue;

        const float MaxFillDepth = FMath::Min(Cell->Terrain.ExcavatedDepthMeters, FMath::Max(0.0f, Cell->Terrain.BedrockDepthMeters - Cell->Terrain.SoilDepthMeters));
        const float RequestedFillDepth = FMath::Min(MaxFillDepth, MaximumDigDepthPerActionMeters * Candidate.Weight);
        const float NeededLoose = RequestedFillDepth * CellArea * FMath::Max(1.0f, LooseSoilExpansionRatio);
        const float ConsumedLoose = FMath::Min(LooseRemaining, NeededLoose);
        const float FillDepth = FMath::Min(RequestedFillDepth, ConsumedLoose / (CellArea * FMath::Max(1.0f, LooseSoilExpansionRatio)));
        if (FillDepth <= KINDA_SMALL_NUMBER) continue;

        Cell->Terrain.Height += FillDepth;
        Cell->Terrain.Deformation += FillDepth;
        Cell->Terrain.SoilDepthMeters += FillDepth;
        Cell->Terrain.ExcavatedDepthMeters = FMath::Max(0.0f, Cell->Terrain.ExcavatedDepthMeters - FillDepth);
        Cell->Terrain.GroundTreatment = Cell->Terrain.ExcavatedDepthMeters > 0.01f ? EWorldGroundTreatment::Disturbed : EWorldGroundTreatment::Natural;
        Cell->bIsDirty = true;
        Chunk->MarkDirty();
        const float ActualLoose = FillDepth * CellArea * FMath::Max(1.0f, LooseSoilExpansionRatio);
        LooseRemaining -= ActualLoose;
        Result.SoilConsumedM3 += ActualLoose;
        Result.FilledVolumeM3 += FillDepth * CellArea;
        ++Result.CellsChanged;
        ChangedChunks.Add(ChunkCoord);
    }

    Result.bSucceeded = Result.CellsChanged > 0;
    if (!Result.bSucceeded) Result.FailureReason = FText::FromString(TEXT("No excavated soil cells were available to fill."));
    Result.AffectedChunks = ChangedChunks.Array();
    Result.AffectedChunks.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X == B.X ? A.Y < B.Y : A.X < B.X; });
    BroadcastChange(ChangedChunks, Result.CellsChanged, 0.0f, Result.FilledVolumeM3);
    return Result;
}

FDigOperationResult UDiggableTerrainSubsystem::PrepareSoilForPlanting(const FVector& WorldLocation, float RadiusMeters, EDigBrushShape Shape)
{
    FDigOperationResult Result;
    UWorldStateSubsystem* State = WorldState.Get();
    if (!State) { Result.FailureReason = FText::FromString(TEXT("World state subsystem is unavailable.")); return Result; }
    TArray<FCellCandidate> Candidates;
    GatherCandidates(WorldLocation, RadiusMeters, Shape, Candidates);
    TSet<FIntPoint> ChangedChunks;

    for (const FCellCandidate& Candidate : Candidates)
    {
        UChunkState* Chunk = nullptr;
        FChunkCell* Cell = nullptr;
        FIntPoint ChunkCoord;
        if (!ResolveExistingCell(Candidate.WorldCenter, Chunk, Cell, ChunkCoord) || !Cell) continue;
        if (Cell->Terrain.SoilTexture == EWorldSoilTexture::Rock || Cell->Terrain.SoilDepthMeters < MinimumSoilDepthMeters) continue;
        Cell->Terrain.Compaction = FMath::Clamp(Cell->Terrain.Compaction - TillageCompactionReduction * Candidate.Weight, 0.0f, 1.0f);
        Cell->Terrain.GroundTreatment = EWorldGroundTreatment::Tilled;
        Cell->bIsDirty = true;
        Chunk->MarkDirty();
        ++Result.CellsChanged;
        ChangedChunks.Add(ChunkCoord);
    }

    Result.bSucceeded = Result.CellsChanged > 0;
    if (!Result.bSucceeded) Result.FailureReason = FText::FromString(TEXT("No suitable soil cells were found for tillage."));
    Result.AffectedChunks = ChangedChunks.Array();
    Result.AffectedChunks.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X == B.X ? A.Y < B.Y : A.X < B.X; });
    BroadcastChange(ChangedChunks, Result.CellsChanged, 0.0f, 0.0f);
    return Result;
}

bool UDiggableTerrainSubsystem::CanPlantAtWorld(const FVector& WorldLocation, float RequiredRootDepthMeters, FText& OutReason) const
{
    UChunkState* Chunk = nullptr;
    FChunkCell* Cell = nullptr;
    FIntPoint ChunkCoord;
    if (!ResolveExistingCell(WorldLocation, Chunk, Cell, ChunkCoord) || !Cell)
    {
        OutReason = FText::FromString(TEXT("Terrain cell is not loaded."));
        return false;
    }
    if (Cell->Terrain.SoilTexture == EWorldSoilTexture::Rock)
    {
        OutReason = FText::FromString(TEXT("Bedrock cannot be planted."));
        return false;
    }
    if (Cell->Terrain.SoilDepthMeters < FMath::Max(MinimumSoilDepthMeters, RequiredRootDepthMeters))
    {
        OutReason = FText::FromString(TEXT("Remaining soil depth is insufficient for the required root zone."));
        return false;
    }
    if (Cell->Terrain.Compaction >= 0.9f)
    {
        OutReason = FText::FromString(TEXT("Soil is too compacted; till it before planting."));
        return false;
    }
    if (Cell->SoilPH < 4.0f || Cell->SoilPH > 9.0f)
    {
        OutReason = FText::FromString(TEXT("Soil pH is outside the supported crop range."));
        return false;
    }
    OutReason = FText::GetEmpty();
    return true;
}

bool UDiggableTerrainSubsystem::SetGroundTreatmentAtWorld(const FVector& WorldLocation, EWorldGroundTreatment Treatment)
{
    UChunkState* Chunk = nullptr;
    FChunkCell* Cell = nullptr;
    FIntPoint ChunkCoord;
    if (!ResolveExistingCell(WorldLocation, Chunk, Cell, ChunkCoord) || !Cell) return false;
    if (Cell->Terrain.SoilTexture == EWorldSoilTexture::Rock && Treatment != EWorldGroundTreatment::Natural) return false;
    Cell->Terrain.GroundTreatment = Treatment;
    Cell->bIsDirty = true;
    Chunk->MarkDirty();
    TSet<FIntPoint> Changed;
    Changed.Add(ChunkCoord);
    BroadcastChange(Changed, 1, 0.0f, 0.0f);
    return true;
}
