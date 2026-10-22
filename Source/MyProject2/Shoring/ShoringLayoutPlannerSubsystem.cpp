#include "ShoringLayoutPlannerSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Shoring/PlaceableShoringPiece.h"

bool UShoringLayoutPlannerSubsystem::BuildTrenchPlan(FVector StartTopCm, FVector EndTopCm,
    float WidthCm, float DepthCm, float StationSpacingCm,
    TArray<FShoringPlacementCandidate>& OutCandidates, FString& OutFailure) const
{
    OutCandidates.Reset();
    OutFailure.Reset();

    if (!FMath::IsFinite(StartTopCm.X) || !FMath::IsFinite(StartTopCm.Y) || !FMath::IsFinite(StartTopCm.Z)
        || !FMath::IsFinite(EndTopCm.X) || !FMath::IsFinite(EndTopCm.Y) || !FMath::IsFinite(EndTopCm.Z)
        || !FMath::IsFinite(WidthCm) || !FMath::IsFinite(DepthCm) || !FMath::IsFinite(StationSpacingCm)
        || WidthCm < 50.0f || DepthCm < 50.0f || StationSpacingCm < 25.0f)
    {
        OutFailure = TEXT("Invalid trench dimensions or station spacing.");
        return false;
    }

    FVector Axis(EndTopCm.X - StartTopCm.X, EndTopCm.Y - StartTopCm.Y, 0.0f);
    const float Length = Axis.Size();
    if (Length < 1.0f)
    {
        OutFailure = TEXT("Trench plan requires a non-zero horizontal length.");
        return false;
    }
    Axis /= Length;
    const FVector Side = FVector::CrossProduct(FVector::UpVector, Axis).GetSafeNormal();
    if (Side.IsNearlyZero())
    {
        OutFailure = TEXT("Could not derive trench side axis.");
        return false;
    }

    const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Axis.Y, Axis.X));
    const float SideYaw = FMath::RadiansToDegrees(FMath::Atan2(Side.Y, Side.X));
    const int32 StationCount = FMath::CeilToInt(Length / StationSpacingCm) + 1;
    const int32 BoardRows = FMath::CeilToInt(DepthCm / FMath::Max(5.0f, WallBoardHeightCm));
    const int32 StrutRows = FMath::Max(1, FMath::FloorToInt((DepthCm - 20.0f) / FMath::Max(10.0f, StrutVerticalSpacingCm)) + 1);
    const int64 Estimated = static_cast<int64>(StationCount) * (2 + 2 * BoardRows + StrutRows);
    if (Estimated <= 0 || Estimated > 10000)
    {
        OutFailure = TEXT("Plan exceeds the 10,000-piece safety limit.");
        return false;
    }

    TArray<float> StationDistances;
    StationDistances.Reserve(StationCount);
    for (int32 I = 0; I < StationCount; ++I)
    {
        StationDistances.Add(FMath::Min(Length, I * StationSpacingCm));
    }
    if (!FMath::IsNearlyEqual(StationDistances.Last(), Length, 1.0f))
    {
        StationDistances.Add(Length);
    }

    auto AddCandidate = [&OutCandidates](EShoringPieceType Type, FVector Location, float PieceYaw,
        FVector Dimensions, int32 Index, int32 SideIndex, FName Role)
    {
        FShoringPlacementCandidate C;
        C.CandidateId = FGuid::NewGuid();
        C.PieceType = Type;
        C.Transform = FTransform(FRotator(0.0f, PieceYaw, 0.0f), Location);
        C.DimensionsCm = Dimensions;
        C.AnchorId = FName(*FString::Printf(TEXT("%s_%d_%d"), *Role.ToString(), Index, SideIndex));
        C.bPlanValid = true;
        C.Message = TEXT("Layout candidate only; commit performs dynamic-object overlap validation.");
        OutCandidates.Add(MoveTemp(C));
    };

    // Uprights at each station on both walls.
    for (int32 I = 0; I < StationDistances.Num(); ++I)
    {
        const float Distance = StationDistances[I];
        const FVector Centre = StartTopCm + Axis * Distance;
        for (int32 WallSide = -1; WallSide <= 1; WallSide += 2)
        {
            const FVector Wall = Centre + Side * (WallSide * WidthCm * 0.5f);
            AddCandidate(EShoringPieceType::Upright,
                FVector(Wall.X, Wall.Y, StartTopCm.Z - DepthCm * 0.5f),
                Yaw, FVector(UprightThicknessCm, UprightThicknessCm, DepthCm), I, WallSide, TEXT("UPRIGHT"));
        }
    }

    // Horizontal boards (lagging) between stations at regular vertical intervals.
    for (int32 I = 0; I + 1 < StationDistances.Num(); ++I)
    {
        const float DistanceA = StationDistances[I];
        const float DistanceB = StationDistances[I + 1];
        const float SegmentLength = DistanceB - DistanceA;
        if (SegmentLength < 1.0f) continue;
        const FVector SegmentCentre = StartTopCm + Axis * ((DistanceA + DistanceB) * 0.5f);
        for (int32 Row = 0; Row < BoardRows; ++Row)
        {
            const float BoardZ = StartTopCm.Z - (Row + 0.5f) * WallBoardHeightCm;
            if (BoardZ < StartTopCm.Z - DepthCm + WallBoardHeightCm * 0.25f) continue;
            for (int32 WallSide = -1; WallSide <= 1; WallSide += 2)
            {
                const FVector Wall = SegmentCentre + Side * (WallSide * WidthCm * 0.5f);
                AddCandidate(EShoringPieceType::WallBoard, FVector(Wall.X, Wall.Y, BoardZ),
                    Yaw, FVector(SegmentLength, WallBoardThicknessCm, WallBoardHeightCm),
                    I * 100 + Row, WallSide, TEXT("BOARD"));
            }
        }
    }

    // Cross-struts. Keep a top clearance and distribute them down the wall.
    for (int32 I = 0; I < StationDistances.Num(); ++I)
    {
        const FVector Centre = StartTopCm + Axis * StationDistances[I];
        for (int32 Row = 0; Row < StrutRows; ++Row)
        {
            const float StrutZ = StartTopCm.Z - (Row + 0.5f) * StrutVerticalSpacingCm;
            if (StrutZ <= StartTopCm.Z - DepthCm + StrutThicknessCm) continue;
            AddCandidate(EShoringPieceType::Strut, FVector(Centre.X, Centre.Y, StrutZ),
                SideYaw, FVector(WidthCm, StrutThicknessCm, StrutThicknessCm),
                I * 100 + Row, 0, TEXT("STRUT"));
        }
    }

    return !OutCandidates.IsEmpty();
}

bool UShoringLayoutPlannerSubsystem::CommitPlan(const TArray<FShoringPlacementCandidate>& Candidates,
    AActor* ActorToIgnore, TArray<APlaceableShoringPiece*>& OutSpawnedPieces, FString& OutFailure) const
{
    OutSpawnedPieces.Reset();
    OutFailure.Reset();
    UWorld* World = GetWorld();
    if (!World || Candidates.IsEmpty())
    {
        OutFailure = TEXT("World or placement candidates are missing.");
        return false;
    }

    // Validate all candidates before spawning: bad data must not create a partial build.
    for (const FShoringPlacementCandidate& Candidate : Candidates)
    {
        if (!Candidate.bPlanValid
            || Candidate.DimensionsCm.X <= 0.0f || Candidate.DimensionsCm.Y <= 0.0f || Candidate.DimensionsCm.Z <= 0.0f
            || !FMath::IsFinite(Candidate.Transform.GetLocation().X)
            || !FMath::IsFinite(Candidate.Transform.GetLocation().Y)
            || !FMath::IsFinite(Candidate.Transform.GetLocation().Z))
        {
            OutFailure = TEXT("Plan contains an invalid placement candidate.");
            return false;
        }
    }

    TArray<APlaceableShoringPiece*> Spawned;
    Spawned.Reserve(Candidates.Num());
    for (const FShoringPlacementCandidate& Candidate : Candidates)
    {
        FCollisionQueryParams Params(SCENE_QUERY_STAT(ShoringPlacementOverlap), false);
        if (IsValid(ActorToIgnore)) Params.AddIgnoredActor(ActorToIgnore);
        for (APlaceableShoringPiece* ExistingInBatch : Spawned)
        {
            if (IsValid(ExistingInBatch)) Params.AddIgnoredActor(ExistingInBatch);
        }

        const FVector HalfExtent = Candidate.DimensionsCm * 0.46f;
        TArray<FOverlapResult> Overlaps;
        const bool bOverlaps = World->OverlapMultiByChannel(Overlaps, Candidate.Transform.GetLocation(),
            Candidate.Transform.GetRotation(), ECC_WorldDynamic,
            FCollisionShape::MakeBox(HalfExtent), Params);

        for (const FOverlapResult& Overlap : Overlaps)
        {
            if (Overlap.GetActor() && !Overlap.GetActor()->IsA<APlaceableShoringPiece>())
            {
                OutFailure = FString::Printf(TEXT("Placement blocked by dynamic actor %s."), *Overlap.GetActor()->GetName());
                for (APlaceableShoringPiece* Piece : Spawned) if (IsValid(Piece)) Piece->Destroy();
                return false;
            }
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        APlaceableShoringPiece* Piece = World->SpawnActor<APlaceableShoringPiece>(
            APlaceableShoringPiece::StaticClass(), Candidate.Transform, SpawnParams);
        if (!Piece)
        {
            OutFailure = TEXT("Could not create a shoring piece; rolled back the whole plan.");
            for (APlaceableShoringPiece* SpawnedPiece : Spawned) if (IsValid(SpawnedPiece)) SpawnedPiece->Destroy();
            return false;
        }
        Piece->InitializeFromCandidate(Candidate);
        Spawned.Add(Piece);
    }

    OutSpawnedPieces = MoveTemp(Spawned);
    return true;
}
