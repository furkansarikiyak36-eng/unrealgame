#include "StakeWallComponent.h"

#include "StakeActor.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Components/StaticMeshComponent.h"

UStakeWallComponent::UStakeWallComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UStakeWallComponent::BuildStakeWall(const FVector& Start, const FVector& End, int32 StakeCount)
{
	UWorld* World = GetWorld();
	if (!World || !StakeClass || StakeCount < 2 || StakeCount > 256
		|| Start.ContainsNaN() || End.ContainsNaN() || !FMath::IsFinite(StakeSpacing)
		|| !FMath::IsFinite(StakeHeight) || StakeSpacing <= 0.f || StakeHeight <= 0.f)
	{
		return false;
	}

	const FVector Segment = End - Start;
	const float Length = Segment.Size();
	if (!FMath::IsFinite(Length) || Length <= KINDA_SMALL_NUMBER
		|| Length / static_cast<float>(StakeCount - 1) < StakeSpacing)
	{
		return false;
	}

	TArray<AStakeActor*> NewStakes;
	NewStakes.Reserve(StakeCount);
	for (int32 Index = 0; Index < StakeCount; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(StakeCount - 1);
		const FVector Sample = FMath::Lerp(Start, End, Alpha);
		FHitResult GroundHit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(StakeWallGround), false, GetOwner());
		const FVector TraceStart = Sample + FVector(0.f, 0.f, 200.f);
		const FVector TraceEnd = Sample - FVector(0.f, 0.f, 500.f);
		if (!World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, Params)
			|| GroundHit.ImpactNormal.Z < 0.6f)
		{
			for (AStakeActor* Spawned : NewStakes) if (IsValid(Spawned)) Spawned->Destroy();
			return false;
		}

		AStakeActor* Stake = World->SpawnActor<AStakeActor>(
			StakeClass, FTransform(FRotator(0.f, Segment.Rotation().Yaw, 0.f), GroundHit.ImpactPoint));
		if (!IsValid(Stake) || !Stake->SetStakeHeight(StakeHeight))
		{
			if (IsValid(Stake)) Stake->Destroy();
			for (AStakeActor* Spawned : NewStakes) if (IsValid(Spawned)) Spawned->Destroy();
			return false;
		}

		// Compensate for either a centred or a base-origin mesh so the bottom rests on the sampled ground.
		const UStaticMeshComponent* Mesh = Stake->StakeMesh;
		const FBox LocalBounds = Mesh->GetStaticMesh()->GetBoundingBox();
		const float ZScale = Mesh->GetRelativeScale3D().Z;
		const float LocalMinZ = LocalBounds.Min.Z * ZScale;
		Stake->SetActorLocation(GroundHit.ImpactPoint - FVector(0.f, 0.f, LocalMinZ), false, nullptr, ETeleportType::TeleportPhysics);
		Stake->Sharpen();
		NewStakes.Add(Stake);
	}

	// Replace the previous wall only after every new stake has spawned and passed placement validation.
	for (AStakeActor* Existing : Stakes)
	{
		if (IsValid(Existing)) Existing->Destroy();
	}
	Stakes.Reset();
	for (AStakeActor* Stake : NewStakes) Stakes.Add(Stake);
	return Stakes.Num() == StakeCount;
}
