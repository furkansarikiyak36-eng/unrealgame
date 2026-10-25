// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldSnapResolver.h"

bool FWorldSnapResolver::Resolve(const TArray<WorldSnap::FPoint>& Candidates,
	const WorldSnap::FQuery& Query, WorldSnap::FResolveResult& OutResult)
{
	OutResult = WorldSnap::FResolveResult{};

	if (Candidates.IsEmpty())
	{
		OutResult.Status = WorldSnap::EResolveStatus::NoCandidates;
		return false;
	}

	if (Query.HitPoint.ContainsNaN() || Query.HitNormal.ContainsNaN()
		|| !FMath::IsFinite(Query.MaxDistanceCm)
		|| !FMath::IsFinite(Query.NormalAlignmentThreshold)
		|| !FMath::IsFinite(Query.MaxAlongAxisErrorCm)
		|| !FMath::IsFinite(Query.AlongAxisWeight)
		|| !FMath::IsFinite(Query.AcrossAxisWeight)
		|| !FMath::IsFinite(Query.NormalPenalty)
		|| Query.MaxDistanceCm < 0.f
		|| Query.MaxAlongAxisErrorCm < 0.f
		|| Query.AlongAxisWeight < 0.f
		|| Query.AcrossAxisWeight < 0.f
		|| Query.NormalPenalty < 0.f)
	{
		OutResult.Status = WorldSnap::EResolveStatus::InvalidQuery;
		return false;
	}

	const FVector QueryNormal = Query.HitNormal.GetSafeNormal();
	const bool bHasQueryNormal = !QueryNormal.IsNearlyZero();
	const float MinNormalDot = FMath::Clamp(Query.NormalAlignmentThreshold, -1.f, 1.f);
	float BestScore = TNumericLimits<float>::Max();
	bool bFound = false;

	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const WorldSnap::FPoint& Candidate = Candidates[Index];

		if ((Query.bRejectUnavailable && !Candidate.bAvailable)
			|| (Query.bRejectOccupied && Candidate.bOccupied)
			|| (Query.bRequireLoadBearing && !Candidate.bLoadBearing))
		{
			continue;
		}
		if (Candidate.Position.ContainsNaN() || Candidate.Normal.ContainsNaN()
			|| Candidate.Axis.ContainsNaN())
		{
			continue;
		}

		if (!Query.RequiredGroup.IsNone() && Candidate.Group != Query.RequiredGroup)
		{
			continue;
		}

		const FVector Delta = Query.HitPoint - Candidate.Position;
		const float DistanceSquared = Delta.SizeSquared();
		const float Distance = FMath::Sqrt(DistanceSquared);
		if (Query.MaxDistanceCm > 0.f && Distance > Query.MaxDistanceCm)
		{
			continue;
		}

		const FVector CandidateNormal = Candidate.Normal.GetSafeNormal();
		float NormalDot = 1.f;
		if (bHasQueryNormal && Query.bRequireNormalCompatibility)
		{
			if (CandidateNormal.IsNearlyZero())
			{
				continue;
			}
			NormalDot = FVector::DotProduct(QueryNormal, CandidateNormal);
			if (NormalDot < MinNormalDot)
			{
				continue;
			}
		}
		else if (bHasQueryNormal && !CandidateNormal.IsNearlyZero())
		{
			NormalDot = FVector::DotProduct(QueryNormal, CandidateNormal);
		}

		float AlongAxisError = 0.f;
		float Score = DistanceSquared;
		if (Query.bUseAxisConstraint)
		{
			const FVector CandidateAxis = Candidate.Axis.GetSafeNormal();
			if (CandidateAxis.IsNearlyZero())
			{
				continue;
			}

			AlongAxisError = FMath::Abs(FVector::DotProduct(Delta, CandidateAxis));
			if (AlongAxisError > Query.MaxAlongAxisErrorCm)
			{
				continue;
			}

			const FVector AcrossAxisDelta = Delta - CandidateAxis * FVector::DotProduct(Delta, CandidateAxis);
			Score = FMath::Square(AlongAxisError) * Query.AlongAxisWeight
				+ AcrossAxisDelta.SizeSquared() * Query.AcrossAxisWeight;
		}

		if (bHasQueryNormal && Query.bRequireNormalCompatibility)
		{
			Score += (1.f - FMath::Clamp(NormalDot, -1.f, 1.f)) * Query.NormalPenalty;
		}

		// Strict comparison intentionally preserves generation order on equal scores.
		if (Score < BestScore)
		{
			BestScore = Score;
			OutResult.bValid = true;
			OutResult.Status = WorldSnap::EResolveStatus::Resolved;
			OutResult.CandidateIndex = Index;
			OutResult.Point = Candidate;
			OutResult.DistanceCm = Distance;
			OutResult.NormalDot = NormalDot;
			OutResult.AlongAxisErrorCm = AlongAxisError;
			OutResult.Score = Score;
			bFound = true;
		}
	}

	if (!bFound)
	{
		OutResult = WorldSnap::FResolveResult{};
		OutResult.Status = WorldSnap::EResolveStatus::NoCompatiblePoint;
		return false;
	}

	return true;
}
