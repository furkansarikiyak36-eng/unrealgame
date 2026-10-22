// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldSnapScanBridge.h"

#include "CarvableLog.h"

namespace
{
	FName GetFaceGroup(LogSnapSystem::EFace Face)
	{
		switch (Face)
		{
		case LogSnapSystem::EFace::Top: return TEXT("Top");
		case LogSnapSystem::EFace::Bottom: return TEXT("Bottom");
		case LogSnapSystem::EFace::Front: return TEXT("Front");
		case LogSnapSystem::EFace::Back: return TEXT("Back");
		case LogSnapSystem::EFace::EndA: return TEXT("EndA");
		case LogSnapSystem::EFace::EndB: return TEXT("EndB");
		default: return NAME_None;
		}
	}
}

bool WorldSnapScanBridge::ResolveLogSnap(const WorldScanSystem::FWorldScanBundle& Scan,
	const WorldSnapScanBridge::FLogSnapSettings& Settings,
	WorldSnapScanBridge::FResolvedLogSnap& OutResult)
{
	OutResult = FResolvedLogSnap{};

	// A query result is meaningful only when the scanner actually attempted and hit with its raycast.
	if (!Scan.bRaycastAttempted || !Scan.Raycast.bHit)
	{
		return false;
	}

	const FHitResult& Hit = Scan.Raycast.Hit;
	ACarvableLog* TargetLog = Cast<ACarvableLog>(Hit.GetActor());
	if (!IsValid(TargetLog))
	{
		return false;
	}

	TArray<LogSnapSystem::FPoint> LogPoints;
	if (!LogSnapSystem::BuildPoints(TargetLog, LogPoints) || LogPoints.IsEmpty())
	{
		return false;
	}

	TArray<WorldSnap::FPoint> Candidates;
	Candidates.Reserve(LogPoints.Num());
	for (int32 Index = 0; Index < LogPoints.Num(); ++Index)
	{
		const LogSnapSystem::FPoint& LogPoint = LogPoints[Index];
		WorldSnap::FPoint& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.Id = LogPoint.Id;
		Candidate.Group = GetFaceGroup(LogPoint.Face);
		Candidate.Position = LogPoint.Position;
		Candidate.Normal = LogPoint.Normal;
		Candidate.Axis = LogPoint.Axis;
		Candidate.SourceIndex = Index;
	}

	WorldSnap::FQuery Query;
	Query.HitPoint = Scan.TargetAnalysis.bValid ? Scan.TargetAnalysis.Point : Hit.ImpactPoint;
	Query.HitNormal = Scan.TargetAnalysis.bValid ? Scan.TargetAnalysis.Normal : Hit.ImpactNormal;
	Query.MaxDistanceCm = Settings.MaxDistanceCm;
	Query.RequiredGroup = Settings.RequiredFaceGroup;
	Query.bRequireNormalCompatibility = true;
	Query.NormalAlignmentThreshold = Settings.NormalAlignmentThreshold;
	Query.bUseAxisConstraint = true;
	Query.MaxAlongAxisErrorCm = Settings.MaxAlongAxisErrorCm;

	WorldSnap::FResolveResult ResolverResult;
	if (!FWorldSnapResolver::Resolve(Candidates, Query, ResolverResult)
		|| !LogPoints.IsValidIndex(ResolverResult.Point.SourceIndex))
	{
		return false;
	}

	OutResult.bValid = true;
	OutResult.SourceHit = Hit;
	OutResult.TargetLog = TargetLog;
	OutResult.TargetAnalysis = Scan.TargetAnalysis;
	if (!OutResult.TargetAnalysis.bValid)
	{
		WorldScanSystem::AnalyzeHit(Scan.Raycast, OutResult.TargetAnalysis);
	}
	OutResult.LogPoint = LogPoints[ResolverResult.Point.SourceIndex];
	OutResult.ResolverResult = ResolverResult;
	return true;
}
