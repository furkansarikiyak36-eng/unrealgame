// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShoringLayoutPlanner.h"

namespace
{
    void AddConnection(
        FShoringLayoutPlan& Plan,
        const TMap<FGuid, int32>& PieceIndexById,
        const FGuid& A,
        const FGuid& B)
    {
        if (!A.IsValid() || !B.IsValid() || A == B)
        {
            return;
        }

        const int32* IndexA = PieceIndexById.Find(A);
        const int32* IndexB = PieceIndexById.Find(B);
        if (!IndexA || !IndexB || !Plan.Pieces.IsValidIndex(*IndexA) || !Plan.Pieces.IsValidIndex(*IndexB))
        {
            return;
        }

        Plan.Pieces[*IndexA].ConnectedPieceIds.AddUnique(B);
        Plan.Pieces[*IndexB].ConnectedPieceIds.AddUnique(A);
    }

    FShoringPieceSpec MakePiece(
        const FGuid& PlanId,
        EShoringPieceKind Kind,
        const FTransform& Transform,
        const FVector& HalfExtentsCm,
        EShoringAnchorKind AnchorKind,
        const FVector& AnchorPositionCm,
        const FVector& AnchorNormal,
        uint64 DensityRevision,
        const FGuid& ConnectedPieceId = FGuid())
    {
        FShoringPieceSpec Piece;
        Piece.PieceId = FGuid::NewGuid();
        Piece.PlanId = PlanId;
        Piece.Kind = Kind;
        Piece.Transform = Transform;
        Piece.HalfExtentsCm = HalfExtentsCm;
        Piece.SupportAnchor.AnchorId = FGuid::NewGuid();
        Piece.SupportAnchor.Kind = AnchorKind;
        Piece.SupportAnchor.PositionCm = AnchorPositionCm;
        Piece.SupportAnchor.Normal = AnchorNormal.GetSafeNormal();
        Piece.SupportAnchor.SourceDensityRevision = DensityRevision;
        Piece.SupportAnchor.ConnectedPieceId = ConnectedPieceId;
        return Piece;
    }
}

bool FShoringLayoutPlanner::BuildPlan(
    const FShoringLayoutRequest& Request,
    FShoringLayoutPlan& OutPlan,
    FString& OutError)
{
    OutPlan = FShoringLayoutPlan{};
    OutError.Reset();

    if (!Request.Validate(OutError))
    {
        OutPlan.Error = OutError;
        return false;
    }

    const FVector HorizontalDelta(
        Request.TrenchEndCm.X - Request.TrenchStartCm.X,
        Request.TrenchEndCm.Y - Request.TrenchStartCm.Y,
        0.0f);
    const float LengthCm = HorizontalDelta.Size();
    const FVector Axis = HorizontalDelta.GetSafeNormal();
    const FVector Lateral = FVector::CrossProduct(FVector::UpVector, Axis).GetSafeNormal();
    if (Axis.IsNearlyZero() || Lateral.IsNearlyZero() || !FMath::IsFinite(LengthCm))
    {
        OutError = TEXT("Trench axis could not be normalized.");
        OutPlan.Error = OutError;
        return false;
    }

    const int32 IntervalCount = FMath::Max(1, FMath::CeilToInt(LengthCm / Request.StationSpacingCm));
    const int32 StationCount = IntervalCount + 1;
    const float ActualSpacingCm = LengthCm / static_cast<float>(IntervalCount);
    const int32 CourseCount = FMath::CeilToInt(Request.TrenchDepthCm / Request.LaggingCourseHeightCm);
    const int64 EstimatedPieceCount =
        static_cast<int64>(StationCount) * 2
        + (Request.bIncludeLagging ? static_cast<int64>(IntervalCount) * 2 * CourseCount : 0)
        + (Request.bIncludeStruts ? StationCount : 0)
        + (Request.bIncludePlatform ? 1 : 0)
        + (Request.bIncludeLadder ? 1 : 0);

    if (StationCount > Request.MaxStations || EstimatedPieceCount < 1 || EstimatedPieceCount > Request.MaxPieces)
    {
        OutError = FString::Printf(
            TEXT("The shoring layout needs %lld pieces at %d stations; it exceeds the configured budget."),
            static_cast<long long>(EstimatedPieceCount), StationCount);
        OutPlan.Error = OutError;
        return false;
    }

    OutPlan.PlanId = FGuid::NewGuid();
    OutPlan.SourceDensityRevision = Request.SourceDensityRevision;
    TMap<FGuid, int32> PieceIndexById;
    PieceIndexById.Reserve(static_cast<int32>(EstimatedPieceCount));

    TArray<FVector> StationCenters;
    TArray<float> StationTopZ;
    TArray<FGuid> NegativeSideUprights;
    TArray<FGuid> PositiveSideUprights;
    StationCenters.Reserve(StationCount);
    StationTopZ.Reserve(StationCount);
    NegativeSideUprights.SetNum(StationCount);
    PositiveSideUprights.SetNum(StationCount);

    for (int32 Station = 0; Station < StationCount; ++Station)
    {
        const float Alpha = static_cast<float>(Station) / static_cast<float>(IntervalCount);
        const FVector Center = FMath::Lerp(Request.TrenchStartCm, Request.TrenchEndCm, Alpha);
        StationCenters.Add(Center);
        StationTopZ.Add(Center.Z);
    }

    const float AxisYaw = Axis.Rotation().Yaw;
    for (int32 Station = 0; Station < StationCount; ++Station)
    {
        const FVector Center = StationCenters[Station];
        const float TopZ = StationTopZ[Station];
        for (const int32 Side : { -1, 1 })
        {
            const FVector InwardNormal = Lateral * static_cast<float>(-Side);
            FVector WallAnchor = Center + Lateral * (static_cast<float>(Side) * Request.TrenchWidthCm * 0.5f);
            WallAnchor.Z = TopZ - Request.TrenchDepthCm * 0.5f;

            const FVector PieceCenter = WallAnchor + InwardNormal * (Request.UprightThicknessCm * 0.5f);
            const FTransform Transform(FRotator(0.0f, AxisYaw, 0.0f), PieceCenter);
            FShoringPieceSpec Piece = MakePiece(
                OutPlan.PlanId,
                EShoringPieceKind::Upright,
                Transform,
                FVector(Request.UprightThicknessCm * 0.5f, Request.UprightThicknessCm * 0.5f, Request.TrenchDepthCm * 0.5f),
                EShoringAnchorKind::VoxelSurface,
                WallAnchor,
                InwardNormal,
                Request.SourceDensityRevision);

            const FGuid PieceId = Piece.PieceId;
            PieceIndexById.Add(PieceId, OutPlan.Pieces.Num());
            OutPlan.Pieces.Add(MoveTemp(Piece));
            if (Side < 0)
            {
                NegativeSideUprights[Station] = PieceId;
            }
            else
            {
                PositiveSideUprights[Station] = PieceId;
            }
        }
    }

    if (Request.bIncludeLagging)
    {
        for (int32 Interval = 0; Interval < IntervalCount; ++Interval)
        {
            const FVector Midpoint = (StationCenters[Interval] + StationCenters[Interval + 1]) * 0.5f;
            const float MidTopZ = (StationTopZ[Interval] + StationTopZ[Interval + 1]) * 0.5f;
            for (const int32 Side : { -1, 1 })
            {
                const FVector InwardNormal = Lateral * static_cast<float>(-Side);
                FVector WallMidpoint = Midpoint + Lateral * (static_cast<float>(Side) * Request.TrenchWidthCm * 0.5f);
                for (int32 Course = 0; Course < CourseCount; ++Course)
                {
                    const float CourseDepth = static_cast<float>(Course) * Request.LaggingCourseHeightCm;
                    const float CourseHeight = FMath::Min(
                        Request.LaggingCourseHeightCm,
                        Request.TrenchDepthCm - CourseDepth);
                    if (CourseHeight <= KINDA_SMALL_NUMBER)
                    {
                        continue;
                    }

                    FVector AnchorPosition = WallMidpoint;
                    AnchorPosition.Z = MidTopZ - CourseDepth - CourseHeight * 0.5f;
                    const FVector PieceCenter = AnchorPosition + InwardNormal * (Request.LaggingThicknessCm * 0.5f);
                    const float HalfLength = FMath::Max(5.0f, (ActualSpacingCm - Request.UprightThicknessCm) * 0.5f);
                    const FTransform Transform(FRotator(0.0f, AxisYaw, 0.0f), PieceCenter);
                    const FGuid FirstUpright = Side < 0
                        ? NegativeSideUprights[Interval]
                        : PositiveSideUprights[Interval];
                    const FGuid SecondUpright = Side < 0
                        ? NegativeSideUprights[Interval + 1]
                        : PositiveSideUprights[Interval + 1];

                    FShoringPieceSpec Piece = MakePiece(
                        OutPlan.PlanId,
                        EShoringPieceKind::Lagging,
                        Transform,
                        FVector(HalfLength, Request.LaggingThicknessCm * 0.5f, CourseHeight * 0.5f),
                        EShoringAnchorKind::PieceSocket,
                        AnchorPosition,
                        InwardNormal,
                        Request.SourceDensityRevision,
                        FirstUpright);
                    const FGuid PieceId = Piece.PieceId;
                    PieceIndexById.Add(PieceId, OutPlan.Pieces.Num());
            OutPlan.Pieces.Add(MoveTemp(Piece));
                    AddConnection(OutPlan, PieceIndexById, PieceId, FirstUpright);
                    AddConnection(OutPlan, PieceIndexById, PieceId, SecondUpright);
                }
            }
        }
    }

    if (Request.bIncludeStruts)
    {
        for (int32 Station = 0; Station < StationCount; ++Station)
        {
            const float TopZ = StationTopZ[Station];
            const float VerticalOffset = FMath::Clamp(
                Request.TrenchDepthCm * 0.35f,
                15.0f,
                Request.TrenchDepthCm - 15.0f);
            FVector PieceCenter = StationCenters[Station];
            PieceCenter.Z = TopZ - VerticalOffset;
            const FTransform Transform(FRotator(0.0f, Lateral.Rotation().Yaw, 0.0f), PieceCenter);
            const float HalfSpan = Request.TrenchWidthCm * 0.5f - Request.UprightThicknessCm;
            if (HalfSpan <= 5.0f)
            {
                OutError = TEXT("The trench is too narrow for the configured strut and upright dimensions.");
                OutPlan = FShoringLayoutPlan{};
                OutPlan.Error = OutError;
                return false;
            }
            FShoringPieceSpec Piece = MakePiece(
                OutPlan.PlanId,
                EShoringPieceKind::Strut,
                Transform,
                FVector(HalfSpan, Request.StrutThicknessCm * 0.5f, Request.StrutThicknessCm * 0.5f),
                EShoringAnchorKind::PieceSocket,
                PieceCenter,
                Lateral,
                Request.SourceDensityRevision,
                NegativeSideUprights[Station]);
            const FGuid PieceId = Piece.PieceId;
            PieceIndexById.Add(PieceId, OutPlan.Pieces.Num());
            OutPlan.Pieces.Add(MoveTemp(Piece));
            AddConnection(OutPlan, PieceIndexById, PieceId, NegativeSideUprights[Station]);
            AddConnection(OutPlan, PieceIndexById, PieceId, PositiveSideUprights[Station]);
        }
    }

    if (Request.bIncludePlatform)
    {
        const int32 MidStation = StationCount / 2;
        FVector PieceCenter = StationCenters[MidStation];
        PieceCenter.Z = StationTopZ[MidStation] - Request.TrenchDepthCm + 5.0f;
        const float PlatformLength = FMath::Min(150.0f, LengthCm);
        const FTransform Transform(FRotator(0.0f, AxisYaw, 0.0f), PieceCenter);
        FShoringPieceSpec Piece = MakePiece(
            OutPlan.PlanId,
            EShoringPieceKind::Platform,
            Transform,
            FVector(PlatformLength * 0.5f, Request.TrenchWidthCm * 0.5f - 10.0f, 5.0f),
            EShoringAnchorKind::PieceSocket,
            PieceCenter,
            FVector::UpVector,
            Request.SourceDensityRevision,
            NegativeSideUprights[MidStation]);
        const FGuid PieceId = Piece.PieceId;
        OutPlan.Pieces.Add(MoveTemp(Piece));
        AddConnection(OutPlan, PieceIndexById, PieceId, NegativeSideUprights[MidStation]);
        AddConnection(OutPlan, PieceIndexById, PieceId, PositiveSideUprights[MidStation]);
    }

    if (Request.bIncludeLadder)
    {
        const float AxisOffset = FMath::Min(60.0f, ActualSpacingCm * 0.4f);
        FVector PieceCenter = StationCenters[0] + Axis * AxisOffset
            + Lateral * (Request.TrenchWidthCm * 0.5f - 40.0f);
        PieceCenter.Z = StationTopZ[0] - Request.TrenchDepthCm * 0.5f + 10.0f;
        const FTransform Transform(FRotator(0.0f, Lateral.Rotation().Yaw, 0.0f), PieceCenter);
        FShoringPieceSpec Piece = MakePiece(
            OutPlan.PlanId,
            EShoringPieceKind::Ladder,
            Transform,
            FVector(30.0f, 5.0f, Request.TrenchDepthCm * 0.5f + 10.0f),
            EShoringAnchorKind::PieceSocket,
            PieceCenter,
            Lateral * -1.0f,
            Request.SourceDensityRevision,
            PositiveSideUprights[0]);
        const FGuid PieceId = Piece.PieceId;
        OutPlan.Pieces.Add(MoveTemp(Piece));
        AddConnection(OutPlan, PieceIndexById, PieceId, PositiveSideUprights[0]);
    }

    OutPlan.bValid = true;
    OutPlan.Error.Reset();
    if (!OutPlan.Validate(OutError))
    {
        OutPlan.bValid = false;
        OutPlan.Error = OutError;
        return false;
    }
    return true;
}