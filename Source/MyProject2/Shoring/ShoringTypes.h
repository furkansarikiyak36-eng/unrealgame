// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShoringTypes.generated.h"

UENUM(BlueprintType)
enum class EShoringPieceKind : uint8
{
    Upright,
    Lagging,
    Strut,
    BaseSole,
    Platform,
    Ladder
};

UENUM(BlueprintType)
enum class EShoringPieceState : uint8
{
    Planned,
    Active,
    NeedsRevalidation,
    Invalid,
    Removed
};

UENUM(BlueprintType)
enum class EShoringAnchorKind : uint8
{
    VoxelSurface,
    PieceSocket,
    Ground
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FShoringSupportAnchor
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring|Anchor")
    FGuid AnchorId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring|Anchor")
    EShoringAnchorKind Kind = EShoringAnchorKind::PieceSocket;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring|Anchor")
    FVector PositionCm = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring|Anchor")
    FVector Normal = FVector::UpVector;

    UPROPERTY(VisibleAnywhere, SaveGame, Category = "Shoring|Anchor")
    uint64 SourceDensityRevision = 0;

    /** Primary linked piece for this socket. Additional links are stored on the piece spec. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring|Anchor")
    FGuid ConnectedPieceId;

    bool IsFinite() const
    {
        return AnchorId.IsValid()
            && FMath::IsFinite(PositionCm.X)
            && FMath::IsFinite(PositionCm.Y)
            && FMath::IsFinite(PositionCm.Z)
            && FMath::IsFinite(Normal.X)
            && FMath::IsFinite(Normal.Y)
            && FMath::IsFinite(Normal.Z)
            && !Normal.IsNearlyZero();
    }
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FShoringPieceSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring")
    FGuid PieceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring")
    FGuid PlanId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring")
    EShoringPieceKind Kind = EShoringPieceKind::Upright;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring")
    FTransform Transform = FTransform::Identity;

    /** Local oriented-box half extents in centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring")
    FVector HalfExtentsCm = FVector(10.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring")
    FShoringSupportAnchor SupportAnchor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring")
    TArray<FGuid> ConnectedPieceIds;

    bool Validate(FString& OutError) const
    {
        OutError.Reset();
        const FVector Location = Transform.GetLocation();
        const FVector Scale = Transform.GetScale3D();
        const FQuat Rotation = Transform.GetRotation();
        const double RotationSizeSquared = Rotation.SizeSquared();

        if (!PieceId.IsValid() || !PlanId.IsValid())
        {
            OutError = TEXT("A shoring piece requires valid piece and plan GUIDs.");
            return false;
        }
        if (!SupportAnchor.IsFinite())
        {
            OutError = TEXT("A shoring piece requires a finite support anchor.");
            return false;
        }
        if (!FMath::IsFinite(Location.X) || !FMath::IsFinite(Location.Y) || !FMath::IsFinite(Location.Z)
            || !FMath::IsFinite(Scale.X) || !FMath::IsFinite(Scale.Y) || !FMath::IsFinite(Scale.Z)
            || Scale.X <= 0.0 || Scale.Y <= 0.0 || Scale.Z <= 0.0
            || !FMath::IsFinite(RotationSizeSquared)
            || !FMath::IsNearlyEqual(RotationSizeSquared, 1.0, 0.01))
        {
            OutError = TEXT("Shoring transform must contain finite location/scale and a normalized rotation.");
            return false;
        }
        if (!FMath::IsFinite(HalfExtentsCm.X) || !FMath::IsFinite(HalfExtentsCm.Y)
            || !FMath::IsFinite(HalfExtentsCm.Z)
            || HalfExtentsCm.X <= 0.0f || HalfExtentsCm.Y <= 0.0f || HalfExtentsCm.Z <= 0.0f)
        {
            OutError = TEXT("Shoring piece half-extents must be finite and positive.");
            return false;
        }
        if (SupportAnchor.Kind == EShoringAnchorKind::PieceSocket && !SupportAnchor.ConnectedPieceId.IsValid())
        {
            OutError = TEXT("A piece-socket anchor must identify its primary connected piece.");
            return false;
        }
        return true;
    }
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FShoringLayoutRequest
{
    GENERATED_BODY()

    /** Endpoints describe the trench axis at its top/rim. X/Y define the axis; Z may vary between endpoints. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner")
    FVector TrenchStartCm = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner")
    FVector TrenchEndCm = FVector(1200.0f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "80.0", ClampMax = "1000.0"))
    float TrenchWidthCm = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "50.0", ClampMax = "2000.0"))
    float TrenchDepthCm = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "50.0", ClampMax = "400.0"))
    float StationSpacingCm = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner")
    bool bIncludeLagging = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner")
    bool bIncludeStruts = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner")
    bool bIncludePlatform = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner")
    bool bIncludeLadder = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "2", ClampMax = "64"))
    int32 MaxStations = 64;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "1", ClampMax = "4096"))
    int32 MaxPieces = 512;

    /** Filled by the world subsystem; pure planner tests may supply their own non-zero revision. */
    UPROPERTY(VisibleAnywhere, Category = "Shoring|Planner")
    uint64 SourceDensityRevision = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "5.0", ClampMax = "50.0"))
    float UprightThicknessCm = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "1.0", ClampMax = "20.0"))
    float LaggingThicknessCm = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "20.0", ClampMax = "100.0"))
    float LaggingCourseHeightCm = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoring|Planner", meta = (ClampMin = "5.0", ClampMax = "30.0"))
    float StrutThicknessCm = 10.0f;

    bool Validate(FString& OutError) const
    {
        OutError.Reset();
        const FVector Delta = TrenchEndCm - TrenchStartCm;
        const double HorizontalLength = FMath::Sqrt(
            static_cast<double>(Delta.X) * Delta.X + static_cast<double>(Delta.Y) * Delta.Y);

        if (!FMath::IsFinite(TrenchStartCm.X) || !FMath::IsFinite(TrenchStartCm.Y) || !FMath::IsFinite(TrenchStartCm.Z)
            || !FMath::IsFinite(TrenchEndCm.X) || !FMath::IsFinite(TrenchEndCm.Y) || !FMath::IsFinite(TrenchEndCm.Z)
            || !FMath::IsFinite(HorizontalLength) || HorizontalLength < 100.0 || HorizontalLength > 50000.0)
        {
            OutError = TEXT("Trench endpoints must be finite and define a horizontal span in [100, 50000] cm.");
            return false;
        }
        if (!FMath::IsFinite(TrenchWidthCm) || TrenchWidthCm < 80.0f || TrenchWidthCm > 1000.0f
            || !FMath::IsFinite(TrenchDepthCm) || TrenchDepthCm < 50.0f || TrenchDepthCm > 2000.0f
            || !FMath::IsFinite(StationSpacingCm) || StationSpacingCm < 50.0f || StationSpacingCm > 400.0f)
        {
            OutError = TEXT("Trench width, depth or station spacing is outside the supported range.");
            return false;
        }
        if (MaxStations < 2 || MaxStations > 64 || MaxPieces < 1 || MaxPieces > 4096)
        {
            OutError = TEXT("Planner budgets are outside their supported ranges.");
            return false;
        }
        if (!FMath::IsFinite(UprightThicknessCm) || UprightThicknessCm < 5.0f || UprightThicknessCm > 50.0f
            || !FMath::IsFinite(LaggingThicknessCm) || LaggingThicknessCm < 1.0f || LaggingThicknessCm > 20.0f
            || !FMath::IsFinite(LaggingCourseHeightCm) || LaggingCourseHeightCm < 20.0f || LaggingCourseHeightCm > 100.0f
            || !FMath::IsFinite(StrutThicknessCm) || StrutThicknessCm < 5.0f || StrutThicknessCm > 30.0f)
        {
            OutError = TEXT("Shoring member dimensions are outside their supported ranges.");
            return false;
        }

        const int32 IntervalCount = FMath::Max(1, FMath::CeilToInt(static_cast<float>(HorizontalLength / StationSpacingCm)));
        if (IntervalCount + 1 > MaxStations)
        {
            OutError = TEXT("The requested trench requires more stations than MaxStations permits.");
            return false;
        }
        return true;
    }
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FShoringLayoutPlan
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shoring|Plan")
    FGuid PlanId;

    UPROPERTY(VisibleAnywhere, Category = "Shoring|Plan")
    uint64 SourceDensityRevision = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shoring|Plan")
    TArray<FShoringPieceSpec> Pieces;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shoring|Plan")
    bool bValid = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shoring|Plan")
    FString Error;

    bool Validate(FString& OutError) const
    {
        OutError.Reset();
        if (!bValid || !PlanId.IsValid() || Pieces.IsEmpty())
        {
            OutError = Error.IsEmpty() ? TEXT("Shoring layout plan is not valid or contains no pieces.") : Error;
            return false;
        }

        TMap<FGuid, int32> IndexById;
        IndexById.Reserve(Pieces.Num());
        for (int32 Index = 0; Index < Pieces.Num(); ++Index)
        {
            FString PieceError;
            if (!Pieces[Index].Validate(PieceError))
            {
                OutError = FString::Printf(TEXT("Piece %d is invalid: %s"), Index, *PieceError);
                return false;
            }
            if (Pieces[Index].PlanId != PlanId || IndexById.Contains(Pieces[Index].PieceId))
            {
                OutError = TEXT("Plan contains a piece with a mismatched plan ID or a duplicate piece GUID.");
                return false;
            }
            IndexById.Add(Pieces[Index].PieceId, Index);
        }

        for (const FShoringPieceSpec& Piece : Pieces)
        {
            TSet<FGuid> UniqueLinks;
            for (const FGuid& ConnectedId : Piece.ConnectedPieceIds)
            {
                const int32* ConnectedIndex = IndexById.Find(ConnectedId);
                if (!ConnectedIndex || ConnectedId == Piece.PieceId || UniqueLinks.Contains(ConnectedId))
                {
                    OutError = TEXT("Plan contains a missing, self-referential or duplicate piece connection.");
                    return false;
                }
                UniqueLinks.Add(ConnectedId);
                if (!Pieces[*ConnectedIndex].ConnectedPieceIds.Contains(Piece.PieceId))
                {
                    OutError = TEXT("Piece connections must be reciprocal so the structure graph is deterministic.");
                    return false;
                }
            }
        }
        return true;
    }
};