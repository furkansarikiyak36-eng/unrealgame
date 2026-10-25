#include "StoneShapeRecognitionSubsystem.h"

#include "Campfire.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "StoneCore/StoneDefinition.h"
#include "StoneCore/StoneTypes.h"
#include "StonePhaseA_Mining/StoneActor.h"

#include "Algo/Sort.h"

bool UStoneShapeRecognitionSubsystem::EvaluateHearthRing(
	const TArray<FVector>& GroundPoints,
	FVector& OutCenter,
	float& OutRadiusCm,
	float& OutRmsRadialErrorCm,
	float& OutMaxAngularGapDegrees,
	float MinRadiusCm,
	float MaxRadiusCm,
	float MaxHeightRangeCm,
	float MaxRelativeRadialError,
	float MaxAngularGapDegrees)
{
	OutCenter = FVector::ZeroVector;
	OutRadiusCm = 0.f;
	OutRmsRadialErrorCm = 0.f;
	OutMaxAngularGapDegrees = 360.f;

	if (GroundPoints.Num() < 5 || GroundPoints.Num() > 16
		|| !FMath::IsFinite(MinRadiusCm) || !FMath::IsFinite(MaxRadiusCm)
		|| !FMath::IsFinite(MaxHeightRangeCm) || !FMath::IsFinite(MaxRelativeRadialError)
		|| !FMath::IsFinite(MaxAngularGapDegrees) || MinRadiusCm <= 0.f
		|| MaxRadiusCm < MinRadiusCm || MaxHeightRangeCm < 0.f
		|| MaxRelativeRadialError < 0.f || MaxAngularGapDegrees <= 0.f
		|| MaxAngularGapDegrees > 360.f)
	{
		return false;
	}

	double SumX = 0.0;
	double SumY = 0.0;
	double SumZ = 0.0;
	float MinZ = TNumericLimits<float>::Max();
	float MaxZ = TNumericLimits<float>::Lowest();
	for (const FVector& Point : GroundPoints)
	{
		if (Point.ContainsNaN() || !FMath::IsFinite(Point.X)
			|| !FMath::IsFinite(Point.Y) || !FMath::IsFinite(Point.Z))
		{
			return false;
		}
		SumX += Point.X;
		SumY += Point.Y;
		SumZ += Point.Z;
		MinZ = FMath::Min(MinZ, Point.Z);
		MaxZ = FMath::Max(MaxZ, Point.Z);
	}

	if (MaxZ - MinZ > MaxHeightRangeCm)
	{
		return false;
	}

	const double InvCount = 1.0 / static_cast<double>(GroundPoints.Num());
	OutCenter = FVector(
		static_cast<float>(SumX * InvCount),
		static_cast<float>(SumY * InvCount),
		static_cast<float>(SumZ * InvCount));

	TArray<float> Radii;
	TArray<float> Angles;
	Radii.Reserve(GroundPoints.Num());
	Angles.Reserve(GroundPoints.Num());
	double SumRadius = 0.0;
	for (const FVector& Point : GroundPoints)
	{
		const FVector2D Delta(Point.X - OutCenter.X, Point.Y - OutCenter.Y);
		const float Radius = Delta.Size();
		Radii.Add(Radius);
		SumRadius += Radius;
		Angles.Add(FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)));
	}

	OutRadiusCm = static_cast<float>(SumRadius * InvCount);
	if (!FMath::IsFinite(OutRadiusCm) || OutRadiusCm < KINDA_SMALL_NUMBER)
	{
		return false;
	}

	double SquaredError = 0.0;
	float MinObservedRadius = TNumericLimits<float>::Max();
	float MaxObservedRadius = 0.f;
	for (const float Radius : Radii)
	{
		const float Error = Radius - OutRadiusCm;
		SquaredError += static_cast<double>(Error) * Error;
		MinObservedRadius = FMath::Min(MinObservedRadius, Radius);
		MaxObservedRadius = FMath::Max(MaxObservedRadius, Radius);
	}
	OutRmsRadialErrorCm = FMath::Sqrt(static_cast<float>(SquaredError * InvCount));

	Algo::Sort(Angles);
	float MaxGap = 0.f;
	for (int32 Index = 0; Index < Angles.Num(); ++Index)
	{
		const float Current = Angles[Index];
		const float Next = Index + 1 < Angles.Num() ? Angles[Index + 1] : Angles[0] + 360.f;
		MaxGap = FMath::Max(MaxGap, Next - Current);
	}
	OutMaxAngularGapDegrees = MaxGap;

	const float AllowedRadialError = FMath::Max(8.f, OutRadiusCm * MaxRelativeRadialError);
	return OutRadiusCm >= MinRadiusCm
		&& OutRadiusCm <= MaxRadiusCm
		&& MinObservedRadius >= OutRadiusCm * 0.55f
		&& MaxObservedRadius <= OutRadiusCm * 1.45f
		&& OutRmsRadialErrorCm <= AllowedRadialError
		&& OutMaxAngularGapDegrees <= MaxAngularGapDegrees;
}

bool UStoneShapeRecognitionSubsystem::IsHearthAlreadyPresent(const FVector& Center) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	for (TActorIterator<ACampfire> It(World); It; ++It)
	{
		const ACampfire* Fire = *It;
		if (IsValid(Fire) && FVector::Dist2D(Fire->GetActorLocation(), Center) <= 70.f)
		{
			return true;
		}
	}
	return false;
}

bool UStoneShapeRecognitionSubsystem::TryRecognizeManualFireplace(AStoneActor* NewlyInstalledStone)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(NewlyInstalledStone)
		|| NewlyInstalledStone->GetStructuralState_Implementation() != EStructuralNodeState::Installed
		|| !NewlyInstalledStone->MeshComponent || !NewlyInstalledStone->GetDefinition())
	{
		return false;
	}

	TArray<TPair<AStoneActor*, FVector>> Candidates;
	const FVector NewGround = NewlyInstalledStone->MeshComponent->Bounds.Origin
		- FVector(0.f, 0.f, NewlyInstalledStone->MeshComponent->Bounds.BoxExtent.Z);
	for (TActorIterator<AStoneActor> It(World); It; ++It)
	{
		AStoneActor* Stone = *It;
		if (!IsValid(Stone) || !Stone->MeshComponent || !Stone->GetDefinition()
			|| Stone->GetStructuralState_Implementation() != EStructuralNodeState::Installed)
		{
			continue;
		}

		const FVector HalfSize = Stone->MeshComponent->Bounds.BoxExtent;
		if (HalfSize.Z * 2.f > 80.f)
		{
			continue;
		}

		const FVector Ground = Stone->MeshComponent->Bounds.Origin - FVector(0.f, 0.f, HalfSize.Z);
		if (FVector::Dist2D(Ground, NewGround) <= NeighborhoodRadiusCm)
		{
			Candidates.Emplace(Stone, Ground);
		}
	}

	// Always include the newly placed stone first; then choose a deterministic local set.
	Candidates.Sort([&NewlyInstalledStone, &NewGround](const TPair<AStoneActor*, FVector>& A, const TPair<AStoneActor*, FVector>& B)
	{
		if (A.Key == NewlyInstalledStone && B.Key != NewlyInstalledStone) return true;
		if (B.Key == NewlyInstalledStone && A.Key != NewlyInstalledStone) return false;
		const float DistanceA = FVector::DistSquared2D(A.Value, NewGround);
		const float DistanceB = FVector::DistSquared2D(B.Value, NewGround);
		if (!FMath::IsNearlyEqual(DistanceA, DistanceB)) return DistanceA < DistanceB;
		if (A.Value.X != B.Value.X) return A.Value.X < B.Value.X;
		if (A.Value.Y != B.Value.Y) return A.Value.Y < B.Value.Y;
		return A.Value.Z < B.Value.Z;
	});

	if (Candidates.Num() < MinimumRingStones)
	{
		return false;
	}
	if (Candidates.Num() > MaximumRingStones)
	{
		Candidates.SetNum(MaximumRingStones);
	}

	TArray<FVector> GroundPoints;
	GroundPoints.Reserve(Candidates.Num());
	for (const TPair<AStoneActor*, FVector>& Candidate : Candidates)
	{
		GroundPoints.Add(Candidate.Value);
	}

	FVector Center;
	float RadiusCm = 0.f;
	float RmsErrorCm = 0.f;
	float MaxGapDegrees = 360.f;
	if (!EvaluateHearthRing(
		GroundPoints, Center, RadiusCm, RmsErrorCm, MaxGapDegrees,
		MinimumRingRadiusCm, MaximumRingRadiusCm, MaximumGroundHeightRangeCm,
		MaximumRelativeRadialError, MaximumAngularGapDegrees))
	{
		return false;
	}

	if (IsHearthAlreadyPresent(Center))
	{
		return false;
	}

	return ACampfire::BuildWithExternalStoneRing(World, Center) != nullptr;
}
