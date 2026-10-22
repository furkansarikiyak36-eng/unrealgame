// Copyright Epic Games, Inc. All Rights Reserved.

#include "LogSnapSystem.h"
#include "CarvableLog.h"
#include "WorldSnapResolver.h"

namespace
{
	constexpr float NormalPenalty = 10000.f;

	FVector GetFaceNormal(LogSnapSystem::EFace Face, const FVector& Axis, const FVector& Side)
	{
		switch (Face)
		{
		case LogSnapSystem::EFace::Top: return FVector::UpVector;
		case LogSnapSystem::EFace::Bottom: return -FVector::UpVector;
		case LogSnapSystem::EFace::Front: return Side;
		case LogSnapSystem::EFace::Back: return -Side;
		case LogSnapSystem::EFace::EndA: return -Axis;
		case LogSnapSystem::EFace::EndB: return Axis;
		default: return FVector::UpVector;
		}
	}
}

bool LogSnapSystem::BuildPoints(const ACarvableLog* Log, TArray<FPoint>& OutPoints)
{
	OutPoints.Reset();
	if (!Log) return false;

	FVector Centre, Axis;
	float Radius = 0.f, Half = 0.f;
	if (!ACarvableLog::GetLogAxis(Log, Centre, Axis, Radius, Half) || FMath::Abs(Axis.Z) > 0.3f)
	{
		return false;
	}

	Axis = FVector(Axis.X, Axis.Y, 0.f).GetSafeNormal();
	if (Axis.IsNearlyZero()) return false;
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Axis).GetSafeNormal();
	TArray<float> LocalLines;
	Log->GetModuleLines(LocalLines);
	const float CentreX = Log->GetActorTransform().InverseTransformPosition(Centre).X;

	auto AddPoint = [&OutPoints, &Centre, &Axis, &Side, Radius](EFace Face, float Along, int32 ModuleIndex, bool bEnd)
	{
		FPoint& Point = OutPoints.AddDefaulted_GetRef();
		Point.Face = Face;
		Point.Axis = Axis;
		Point.AlongAxis = Along;
		Point.ModuleIndex = ModuleIndex;
		Point.bEnd = bEnd;
		Point.Normal = GetFaceNormal(Face, Axis, Side);
		Point.Position = Centre + Axis * Along;
		if (!bEnd) Point.Position += Point.Normal * Radius;
		const TCHAR* FaceToken = TEXT("Top");
		switch (Face)
		{
		case EFace::Top: FaceToken = TEXT("Top"); break;
		case EFace::Bottom: FaceToken = TEXT("Bottom"); break;
		case EFace::Front: FaceToken = TEXT("Front"); break;
		case EFace::Back: FaceToken = TEXT("Back"); break;
		case EFace::EndA: FaceToken = TEXT("EndA"); break;
		case EFace::EndB: FaceToken = TEXT("EndB"); break;
		}
		Point.Id = FName(*FString::Printf(TEXT("%s_%02d"), FaceToken, ModuleIndex < 0 ? 0 : ModuleIndex));
	};

	int32 ModuleIndex = 0;
	for (const float LocalLine : LocalLines)
	{
		const float Along = LocalLine - CentreX;
		if (FMath::Abs(Along) >= Half - 1.f) continue;
		AddPoint(EFace::Top, Along, ModuleIndex, false);
		AddPoint(EFace::Bottom, Along, ModuleIndex, false);
		AddPoint(EFace::Front, Along, ModuleIndex, false);
		AddPoint(EFace::Back, Along, ModuleIndex, false);
		++ModuleIndex;
	}
	// One-module logs may have no interior cut/module line; their centre is still a valid support snap.
	if (ModuleIndex == 0)
	{
		AddPoint(EFace::Top, 0.f, 0, false);
		AddPoint(EFace::Bottom, 0.f, 0, false);
		AddPoint(EFace::Front, 0.f, 0, false);
		AddPoint(EFace::Back, 0.f, 0, false);
	}
	AddPoint(EFace::EndA, -Half, INDEX_NONE, true);
	AddPoint(EFace::EndB, Half, INDEX_NONE, true);
	return !OutPoints.IsEmpty();
}

bool LogSnapSystem::Resolve(const TArray<FPoint>& Points, const FQuery& Query, FPoint& OutPoint)
{
    OutPoint = FPoint{};
    if (Points.IsEmpty())
    {
        return false;
    }

    TArray<WorldSnap::FPoint> Candidates;
    Candidates.Reserve(Points.Num());
    for (int32 Index = 0; Index < Points.Num(); ++Index)
    {
        const FPoint& Source = Points[Index];
        WorldSnap::FPoint& Candidate = Candidates.AddDefaulted_GetRef();
        Candidate.Id = Source.Id;
        switch (Source.Face)
        {
        case EFace::Top: Candidate.Group = TEXT("Top"); break;
        case EFace::Bottom: Candidate.Group = TEXT("Bottom"); break;
        case EFace::Front: Candidate.Group = TEXT("Front"); break;
        case EFace::Back: Candidate.Group = TEXT("Back"); break;
        case EFace::EndA: Candidate.Group = TEXT("EndA"); break;
        case EFace::EndB: Candidate.Group = TEXT("EndB"); break;
        }
        Candidate.Position = Source.Position;
        Candidate.Normal = Source.Normal;
        Candidate.Axis = Source.Axis;
        Candidate.SourceIndex = Index;
    }

    WorldSnap::FQuery SharedQuery;
    SharedQuery.HitPoint = Query.HitPoint;
    SharedQuery.HitNormal = Query.HitNormal;
    SharedQuery.MaxDistanceCm = 0.0f; // Preserve the legacy log resolver's axis-limited, non-radial behavior.
    SharedQuery.bRequireNormalCompatibility = true;
    SharedQuery.NormalAlignmentThreshold = Query.NormalAlignmentThreshold;
    SharedQuery.bUseAxisConstraint = true;
    SharedQuery.MaxAlongAxisErrorCm = Query.MaxAlongAxisError;
    SharedQuery.AlongAxisWeight = 1.0f;
    SharedQuery.AcrossAxisWeight = 0.02f;
    SharedQuery.NormalPenalty = 10000.0f;

    WorldSnap::FResolveResult Result;
    if (!FWorldSnapResolver::Resolve(Candidates, SharedQuery, Result)
        || !Points.IsValidIndex(Result.Point.SourceIndex))
    {
        return false;
    }

    OutPoint = Points[Result.Point.SourceIndex];
    return true;
}

float LogSnapSystem::FoldAxisYawDelta(float DeltaDegrees)
{
	float Delta = FMath::FindDeltaAngleDegrees(0.f, DeltaDegrees);
	if (Delta > 90.f) Delta -= 180.f;
	else if (Delta <= -90.f) Delta += 180.f;
	return Delta;
}

float LogSnapSystem::NearestSector(float DeltaDegrees)
{
	const float Delta = FoldAxisYawDelta(DeltaDegrees);
	const float Sectors[] = { 0.f, 45.f, -45.f, 90.f };
	float BestSector = Sectors[0];
	float BestDistance = TNumericLimits<float>::Max();
	for (const float Sector : Sectors)
	{
		const float Distance = Sector == 90.f ? 90.f - FMath::Abs(Delta) : FMath::Abs(Delta - Sector);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			BestSector = Sector;
		}
	}
	return BestSector;
}
