// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"

class AActor;
class UPhysicalMaterial;
class UPrimitiveComponent;

/**
 * Global world-hit acquisition, physical analysis and classification.
 *
 * This layer deliberately does not place objects or resolve snap points. It produces
 * a normalized description of what a trace hit, which downstream systems can consume.
 */
namespace WorldScanSystem
{
	enum class EObjectType : uint8
	{
		Unknown,
		Ground,
		Tree,
		Log,
		Rock,
		Stone,
		RawResource,
		Structure,
		StaticWorldObject
	};

	enum class ESurfaceProfile : uint8
	{
		Unknown,
		Flat,
		Slope,
		SteepSlope,
		Vertical
	};

	enum class EPhysicalState : uint8
	{
		Unknown,
		Static,
		Moving,
		Falling,
		Sleeping
	};

	enum class ECapability : uint8
	{
		None       = 0,
		CanSupport = 1 << 0,
		CanSnap    = 1 << 1,
		CanDig     = 1 << 2,
		CanHarvest = 1 << 3,
		CanCarry   = 1 << 4,
		CanInteract= 1 << 5
	};
	ENUM_CLASS_FLAGS(ECapability)

	struct FScanResult
	{
		bool bHit = false;
		FHitResult Hit;
		FVector TraceStart = FVector::ZeroVector;
		FVector TraceEnd = FVector::ZeroVector;
		float TraceLengthCm = 0.f;
	};

	struct FWorldAnalysis
	{
		bool bValid = false;
		EObjectType ObjectType = EObjectType::Unknown;
		ESurfaceProfile SurfaceProfile = ESurfaceProfile::Unknown;
		EPhysicalState PhysicalState = EPhysicalState::Unknown;
		ECapability Capabilities = ECapability::None;

		FVector Point = FVector::ZeroVector;
		FVector Normal = FVector::UpVector;
		float SurfaceAngleDegrees = 0.f;
		float HitDistanceCm = 0.f;

		TWeakObjectPtr<AActor> Actor;
		TWeakObjectPtr<UPrimitiveComponent> Component;
		TWeakObjectPtr<UPhysicalMaterial> PhysicalMaterial;

		bool HasCapability(ECapability Capability) const
		{
			return EnumHasAllFlags(Capabilities, Capability);
		}
	};

	/** Stage 1: perform one world trace and retain the raw FHitResult. */
	MYPROJECT2_API bool Trace(
		UWorld* World,
		const FVector& Start,
		const FVector& End,
		const AActor* ActorToIgnore,
		FScanResult& OutResult,
		ECollisionChannel Channel = ECC_Visibility,
		bool bTraceComplex = true);

	/** Stage 2: derive geometry and physical state from a successful trace hit. */
	MYPROJECT2_API bool AnalyzeHit(const FScanResult& Scan, FWorldAnalysis& OutAnalysis);

	/** Stage 3: classify the hit using known gameplay actor types and explicit actor/component tags. */
	MYPROJECT2_API EObjectType ClassifyObject(const FHitResult& Hit);
	MYPROJECT2_API ESurfaceProfile ClassifySurface(const FVector& WorldNormal, float* OutAngleDegrees = nullptr);
	MYPROJECT2_API EPhysicalState ClassifyPhysicalState(const FHitResult& Hit);
	MYPROJECT2_API ECapability InferCapabilities(EObjectType ObjectType, EPhysicalState PhysicalState);

	/** Tag contract for generic actors and assets:
	 * World.Object.Tree / Log / Rock / Stone / Ground / Structure / RawResource
	 * World.State.Falling / Moving / Sleeping
	 * World.Capability.NoSupport / NoSnap / Dig / Harvest / Carry / Interact
	 */
	MYPROJECT2_API bool HasWorldTag(const AActor* Actor, FName Tag);
}
