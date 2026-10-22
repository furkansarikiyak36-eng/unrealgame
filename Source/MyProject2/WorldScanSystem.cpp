// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldScanSystem.h"

#include "CarvableLog.h"
#include "HarvestableActor.h"
#include "RawMaterialNode.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "LandscapeProxy.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

namespace
{
	constexpr float FlatAngleDegrees = 12.f;
	constexpr float SteepSlopeAngleDegrees = 50.f;
	constexpr float VerticalAngleDegrees = 75.f;
	constexpr float MotionSpeedCmPerSec = 5.f;
	constexpr float FallingVerticalSpeedCmPerSec = 20.f;

	bool HasTag(const AActor* Actor, const TCHAR* Tag)
	{
		return Actor && Actor->ActorHasTag(FName(Tag));
	}

	bool HasComponentTag(const UPrimitiveComponent* Component, const TCHAR* Tag)
	{
		return Component && Component->ComponentHasTag(FName(Tag));
	}
}

bool WorldScanSystem::HasWorldTag(const AActor* Actor, FName Tag)
{
	return Actor && Actor->ActorHasTag(Tag);
}

bool WorldScanSystem::Trace(
	UWorld* World,
	const FVector& Start,
	const FVector& End,
	const AActor* ActorToIgnore,
	FScanResult& OutResult,
	ECollisionChannel Channel,
	bool bTraceComplex,
	const TArray<const UPrimitiveComponent*>* ComponentsToIgnore)
{
	OutResult = FScanResult{};
	OutResult.TraceStart = Start;
	OutResult.TraceEnd = End;
	OutResult.TraceLengthCm = FVector::Distance(Start, End);

	if (!World || OutResult.TraceLengthCm <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WorldScanSystemTrace), bTraceComplex);
	Params.bReturnPhysicalMaterial = true;
	Params.bFindInitialOverlaps = false;
	if (ActorToIgnore)
	{
		Params.AddIgnoredActor(ActorToIgnore);
	}
	if (ComponentsToIgnore)
	{
		for (const UPrimitiveComponent* Component : *ComponentsToIgnore)
		{
			if (IsValid(Component))
			{
				Params.AddIgnoredComponent(Component);
			}
		}
	}

	OutResult.bHit = World->LineTraceSingleByChannel(OutResult.Hit, Start, End, Channel, Params);
	return OutResult.bHit;
}

bool WorldScanSystem::AnalyzeHit(const FScanResult& Scan, FWorldAnalysis& OutAnalysis)
{
	OutAnalysis = FWorldAnalysis{};
	if (!Scan.bHit || !Scan.Hit.bBlockingHit || !Scan.Hit.GetActor())
	{
		return false;
	}

	const FHitResult& Hit = Scan.Hit;
	OutAnalysis.bValid = true;
	OutAnalysis.Point = Hit.ImpactPoint;
	OutAnalysis.Normal = Hit.ImpactNormal.GetSafeNormal();
	OutAnalysis.HitDistanceCm = Hit.Distance;
	OutAnalysis.Actor = Hit.GetActor();
	OutAnalysis.Component = Hit.GetComponent();
	OutAnalysis.PhysicalMaterial = Hit.PhysMaterial.Get();
	OutAnalysis.ObjectType = ClassifyObject(Hit);
	OutAnalysis.SurfaceProfile = ClassifySurface(OutAnalysis.Normal, &OutAnalysis.SurfaceAngleDegrees);
	OutAnalysis.PhysicalState = ClassifyPhysicalState(Hit);
	OutAnalysis.Capabilities = InferCapabilities(OutAnalysis.ObjectType, OutAnalysis.PhysicalState);

	const AActor* Actor = Hit.GetActor();
	const UPrimitiveComponent* Component = Hit.GetComponent();
	if (HasTag(Actor, TEXT("World.Capability.NoSupport")) || HasComponentTag(Component, TEXT("World.Capability.NoSupport")))
	{
		OutAnalysis.Capabilities &= ~ECapability::CanSupport;
	}
	if (HasTag(Actor, TEXT("World.Capability.NoSnap")) || HasComponentTag(Component, TEXT("World.Capability.NoSnap")))
	{
		OutAnalysis.Capabilities &= ~ECapability::CanSnap;
	}
	if (HasTag(Actor, TEXT("World.Capability.Dig")) || HasComponentTag(Component, TEXT("World.Capability.Dig")))
	{
		OutAnalysis.Capabilities |= ECapability::CanDig;
	}
	if (HasTag(Actor, TEXT("World.Capability.Harvest")) || HasComponentTag(Component, TEXT("World.Capability.Harvest")))
	{
		OutAnalysis.Capabilities |= ECapability::CanHarvest;
	}
	if (HasTag(Actor, TEXT("World.Capability.Carry")) || HasComponentTag(Component, TEXT("World.Capability.Carry")))
	{
		OutAnalysis.Capabilities |= ECapability::CanCarry;
	}
	if (HasTag(Actor, TEXT("World.Capability.Interact")) || HasComponentTag(Component, TEXT("World.Capability.Interact")))
	{
		OutAnalysis.Capabilities |= ECapability::CanInteract;
	}
	return true;
}

WorldScanSystem::EObjectType WorldScanSystem::ClassifyObject(const FHitResult& Hit)
{
	const AActor* Actor = Hit.GetActor();
	const UPrimitiveComponent* Component = Hit.GetComponent();
	if (!Actor)
	{
		return EObjectType::Unknown;
	}

	// Explicit tags allow assets and future actor types to join the global system without casts.
	if (HasTag(Actor, TEXT("World.Object.Tree")) || HasComponentTag(Component, TEXT("World.Object.Tree"))) return EObjectType::Tree;
	if (HasTag(Actor, TEXT("World.Object.Log")) || HasComponentTag(Component, TEXT("World.Object.Log"))) return EObjectType::Log;
	if (HasTag(Actor, TEXT("World.Object.Rock")) || HasComponentTag(Component, TEXT("World.Object.Rock"))) return EObjectType::Rock;
	if (HasTag(Actor, TEXT("World.Object.Stone")) || HasComponentTag(Component, TEXT("World.Object.Stone"))) return EObjectType::Stone;
	if (HasTag(Actor, TEXT("World.Object.Ground")) || HasComponentTag(Component, TEXT("World.Object.Ground"))) return EObjectType::Ground;
	if (HasTag(Actor, TEXT("World.Object.Structure")) || HasComponentTag(Component, TEXT("World.Object.Structure"))) return EObjectType::Structure;
	if (HasTag(Actor, TEXT("World.Object.RawResource")) || HasComponentTag(Component, TEXT("World.Object.RawResource"))) return EObjectType::RawResource;

	if (Cast<ACarvableLog>(Actor)) return EObjectType::Log;
	if (const AHarvestableActor* Harvestable = Cast<AHarvestableActor>(Actor))
	{
		if (Harvestable->IsTree()) return EObjectType::Tree;
		if (Harvestable->IsTrunk()) return EObjectType::Log;
		if (!Harvestable->IsWood()) return EObjectType::Rock;
	}
	if (Cast<AStoneActor>(Actor)) return EObjectType::Stone;
	if (Cast<ARawMaterialNode>(Actor)) return EObjectType::RawResource;
	if (Cast<ALandscapeProxy>(Actor)) return EObjectType::Ground;

	if (Actor->ActorHasTag(FName(TEXT("Fellable")))) return EObjectType::Tree;
	if (Actor->ActorHasTag(FName(TEXT("Breakable")))) return EObjectType::Rock;

	if (Component && Component->Mobility == EComponentMobility::Static)
	{
		return EObjectType::StaticWorldObject;
	}
	return EObjectType::Unknown;
}

WorldScanSystem::ESurfaceProfile WorldScanSystem::ClassifySurface(const FVector& WorldNormal, float* OutAngleDegrees)
{
	const FVector Normal = WorldNormal.GetSafeNormal();
	if (Normal.IsNearlyZero())
	{
		if (OutAngleDegrees) *OutAngleDegrees = 0.f;
		return ESurfaceProfile::Unknown;
	}

	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Normal, FVector::UpVector), -1.f, 1.f)));
	if (OutAngleDegrees) *OutAngleDegrees = Angle;

	// Impact normal points out of the contacted surface. Down-facing ceilings are treated as non-ground surfaces.
	if (Angle <= FlatAngleDegrees) return ESurfaceProfile::Flat;
	if (Angle >= VerticalAngleDegrees && Angle <= 180.f - VerticalAngleDegrees) return ESurfaceProfile::Vertical;
	if (Angle > 90.f) return ESurfaceProfile::Unknown;
	if (Angle >= SteepSlopeAngleDegrees) return ESurfaceProfile::SteepSlope;
	return ESurfaceProfile::Slope;
}

WorldScanSystem::EPhysicalState WorldScanSystem::ClassifyPhysicalState(const FHitResult& Hit)
{
	const AActor* Actor = Hit.GetActor();
	const UPrimitiveComponent* Component = Hit.GetComponent();
	if (!Actor)
	{
		return EPhysicalState::Unknown;
	}

	if (HasTag(Actor, TEXT("World.State.Falling")) || HasComponentTag(Component, TEXT("World.State.Falling")))
	{
		return EPhysicalState::Falling;
	}
	if (HasTag(Actor, TEXT("World.State.Sleeping")) || HasComponentTag(Component, TEXT("World.State.Sleeping")))
	{
		return EPhysicalState::Sleeping;
	}
	if (HasTag(Actor, TEXT("World.State.Moving")) || HasComponentTag(Component, TEXT("World.State.Moving")))
	{
		return EPhysicalState::Moving;
	}

	if (Component && Component->IsSimulatingPhysics())
	{
		if (Component->IsAnyRigidBodyAwake())
		{
			const FVector Velocity = Component->GetPhysicsLinearVelocity();
			if (Velocity.Z < -FallingVerticalSpeedCmPerSec) return EPhysicalState::Falling;
			if (Velocity.SizeSquared() > FMath::Square(MotionSpeedCmPerSec)) return EPhysicalState::Moving;
			return EPhysicalState::Moving;
		}
		return EPhysicalState::Sleeping;
	}

	if (Actor->GetVelocity().SizeSquared() > FMath::Square(MotionSpeedCmPerSec))
	{
		return Actor->GetVelocity().Z < -FallingVerticalSpeedCmPerSec ? EPhysicalState::Falling : EPhysicalState::Moving;
	}
	return Component && Component->Mobility == EComponentMobility::Static
		? EPhysicalState::Static
		: EPhysicalState::Unknown;
}

WorldScanSystem::ECapability WorldScanSystem::InferCapabilities(EObjectType ObjectType, EPhysicalState PhysicalState)
{
	ECapability Result = ECapability::None;
	switch (ObjectType)
	{
	case EObjectType::Ground:
	case EObjectType::Stone:
	case EObjectType::Structure:
	case EObjectType::StaticWorldObject:
		Result |= ECapability::CanSupport;
		break;
	case EObjectType::Tree:
	case EObjectType::Log:
		Result |= ECapability::CanSupport | ECapability::CanSnap | ECapability::CanHarvest | ECapability::CanCarry;
		break;
	case EObjectType::Rock:
		Result |= ECapability::CanSupport | ECapability::CanHarvest | ECapability::CanCarry;
		break;
	case EObjectType::RawResource:
		Result |= ECapability::CanInteract | ECapability::CanHarvest;
		break;
	default:
		break;
	}

	// Moving targets can still be queried, but are not presumed safe structural supports.
	if (PhysicalState == EPhysicalState::Moving || PhysicalState == EPhysicalState::Falling)
	{
		Result &= ~ECapability::CanSupport;
	}
	return Result;
}
