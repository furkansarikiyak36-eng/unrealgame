// Copyright Epic Games, Inc. All Rights Reserved.

#include "LogSnapSystem.h"
#include "CarvableLog.h"

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
	AddPoint(EFace::EndA, -Half, INDEX_NONE, true);
	AddPoint(EFace::EndB, Half, INDEX_NONE, true);
	return !OutPoints.IsEmpty();
}

bool LogSnapSystem::Resolve(const TArray<FPoint>& Points, const FQuery& Query, FPoint& OutPoint)
{
	if (Points.IsEmpty()) return false;
	const FVector HitNormal = Query.HitNormal.GetSafeNormal();
	float BestScore = TNumericLimits<float>::Max();
	bool bFound = false;

	for (const FPoint& Candidate : Points)
	{
		const float AlongError = FMath::Abs(FVector::DotProduct(Query.HitPoint - Candidate.Position, Candidate.Axis));
		if (AlongError > Query.MaxAlongAxisError) continue;
		const float NormalDot = FVector::DotProduct(HitNormal, Candidate.Normal);
		if (!HitNormal.IsNearlyZero() && NormalDot < Query.NormalAlignmentThreshold) continue;

		const FVector Delta = Query.HitPoint - Candidate.Position;
		const FVector AcrossAxis = Delta - Candidate.Axis * FVector::DotProduct(Delta, Candidate.Axis);
		const float Score = AlongError * AlongError + AcrossAxis.SizeSquared() * 0.02f
			+ (HitNormal.IsNearlyZero() ? 0.f : (1.f - FMath::Clamp(NormalDot, -1.f, 1.f)) * NormalPenalty);
		if (Score < BestScore)
		{
			BestScore = Score;
			OutPoint = Candidate;
			bFound = true;
		}
	}
	return bFound;
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
