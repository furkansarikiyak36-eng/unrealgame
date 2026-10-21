// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"

class AActor;
class UPhysicalMaterial;
class UPrimitiveComponent;

/**
 * Shared world sensing primitives. Query methods acquire raw engine data; analysis methods
 * normalize it. Expensive multi-sample or mesh-level work is explicit and demand-driven.
 */
namespace WorldScanSystem
{
	enum class EObjectType : uint8
	{
		Unknown, Ground, Tree, Log, Rock, Stone, RawResource, Structure, StaticWorldObject
	};

	enum class ESurfaceProfile : uint8 { Unknown, Flat, Slope, SteepSlope, Vertical };
	enum class EPhysicalState : uint8 { Unknown, Static, Moving, Falling, Sleeping };

	enum class ECapability : uint8
	{
		None = 0, CanSupport = 1 << 0, CanSnap = 1 << 1, CanDig = 1 << 2,
		CanHarvest = 1 << 3, CanCarry = 1 << 4, CanInteract = 1 << 5
	};
	ENUM_CLASS_FLAGS(ECapability)

	enum class EShapeTraceType : uint8 { Sphere, Box, Capsule };

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

	struct FShapeTraceRequest
	{
		EShapeTraceType Type = EShapeTraceType::Sphere;
		float RadiusCm = 25.f;
		FVector HalfExtentCm = FVector(25.f);
		float HalfHeightCm = 50.f;
	};

	struct FShapeScanResult
	{
		bool bHit = false;
		TArray<FHitResult> Hits;
		FVector TraceStart = FVector::ZeroVector;
		FVector TraceEnd = FVector::ZeroVector;
		float TraceLengthCm = 0.f;
	};

	struct FOverlapScanResult
	{
		bool bAnyOverlap = false;
		TArray<FOverlapResult> Overlaps;
		FVector Center = FVector::ZeroVector;
	};

	struct FGeometrySampleSettings
	{
		FVector2D HalfExtentCm = FVector2D(50.f, 50.f);
		int32 SamplesX = 3;
		int32 SamplesY = 3;
		float TraceUpCm = 200.f;
		float TraceDownCm = 400.f;
		ECollisionChannel Channel = ECC_Visibility;
		bool bTraceComplex = false;
	};

	struct FGeometrySampleResult
	{
		bool bValid = false;
		TArray<FVector> SamplePoints;
		TArray<FVector> SampleNormals;
		FVector AverageNormal = FVector::UpVector;
		float MinHeightCm = 0.f;
		float MaxHeightCm = 0.f;
		float HeightRangeCm = 0.f;
		float MaxSlopeDegrees = 0.f;
		int32 RequestedSamples = 0;
		int32 SuccessfulSamples = 0;
	};

	struct FBoundsAnalysis
	{
		bool bValid = false;
		FBox Bounds = FBox(ForceInit);
		FVector Origin = FVector::ZeroVector;
		FVector Extent = FVector::ZeroVector;
		FVector Dimensions = FVector::ZeroVector;
		float SphereRadiusCm = 0.f;
		FQuat Rotation = FQuat::Identity;
	};

	struct FPhysicsStateAnalysis
	{
		bool bValid = false;
		EPhysicalState State = EPhysicalState::Unknown;
		FVector LinearVelocityCmPerSec = FVector::ZeroVector;
		FVector AngularVelocityRadiansPerSec = FVector::ZeroVector;
		bool bSimulatingPhysics = false;
		bool bRigidBodyAwake = false;
	};

	/** Selective orchestration request. Disabled methods do no work. */
	struct FWorldScanRequest
	{
		bool bRunRaycast = true;
		bool bRunShapeTrace = false;
		bool bRunOverlapSphere = false;
		bool bRunOverlapBox = false;
		bool bRunSurfaceSampling = false;
		bool bRunBoundsAnalysis = true;
		bool bRunPhysicsQuery = true;
		FVector TraceStart = FVector::ZeroVector;
		FVector TraceEnd = FVector::ZeroVector;
		FVector QueryCenter = FVector::ZeroVector;
		ECollisionChannel TraceChannel = ECC_Visibility;
		bool bTraceComplex = false;
		FShapeTraceRequest ShapeRequest;
		FVector ShapeTraceStart = FVector::ZeroVector;
		FVector ShapeTraceEnd = FVector::ZeroVector;
		float OverlapRadiusCm = 150.f;
		FVector OverlapHalfExtentCm = FVector(100.f);
		FQuat OverlapRotation = FQuat::Identity;
		FGeometrySampleSettings GeometrySettings;
	};

	struct FWorldScanBundle
	{
		bool bRaycastAttempted = false;
		bool bShapeTraceAttempted = false;
		bool bOverlapSphereAttempted = false;
		bool bOverlapBoxAttempted = false;
		bool bSurfaceSamplingAttempted = false;
		bool bBoundsAnalysisAttempted = false;
		bool bPhysicsQueryAttempted = false;
		FScanResult Raycast;
		FWorldAnalysis TargetAnalysis;
		FShapeScanResult ShapeTrace;
		FOverlapScanResult OverlapSphere;
		FOverlapScanResult OverlapBox;
		FGeometrySampleResult Surface;
		FBoundsAnalysis Bounds;
		FPhysicsStateAnalysis Physics;
	};

	/** Execute only the methods enabled in the request; raycast hit supplies a default target for bounds/physics. */
	MYPROJECT2_API bool ExecuteScanPlan(UWorld* World, const FWorldScanRequest& Request,
		const AActor* ActorToIgnore, const AActor* TargetActorOverride,
		const UPrimitiveComponent* TargetComponentOverride, FWorldScanBundle& OutBundle);

	/** Method 1: line trace. */
	MYPROJECT2_API bool Trace(UWorld* World, const FVector& Start, const FVector& End,
		const AActor* ActorToIgnore, FScanResult& OutResult,
		ECollisionChannel Channel = ECC_Visibility, bool bTraceComplex = true);

	/** Method 2: swept sphere/box/capsule. Multi-hit is optional to avoid accidental result explosions. */
	MYPROJECT2_API bool ShapeTrace(UWorld* World, const FVector& Start, const FVector& End,
		const FShapeTraceRequest& Request, const AActor* ActorToIgnore, FShapeScanResult& OutResult,
		ECollisionChannel Channel = ECC_Visibility, bool bTraceComplex = false, bool bMulti = false);

	/** Method 3: collect overlaps in a sphere or oriented box. */
	MYPROJECT2_API bool OverlapSphere(UWorld* World, const FVector& Center, float RadiusCm,
		const AActor* ActorToIgnore, FOverlapScanResult& OutResult,
		ECollisionChannel Channel = ECC_WorldDynamic);
	MYPROJECT2_API bool OverlapBox(UWorld* World, const FVector& Center, const FVector& HalfExtentCm,
		const FQuat& Rotation, const AActor* ActorToIgnore, FOverlapScanResult& OutResult,
		ECollisionChannel Channel = ECC_WorldDynamic);

	/** Method 4: grid-sample a terrain/surface patch using downward traces. */
	MYPROJECT2_API bool SampleSurface(UWorld* World, const FVector& Center,
		const FGeometrySampleSettings& Settings, const AActor* ActorToIgnore,
		FGeometrySampleResult& OutResult);

	/** Method 5: read cheap component/actor bounds; does not iterate every mesh triangle. */
	MYPROJECT2_API bool AnalyzeBounds(const AActor* Actor, const UPrimitiveComponent* PreferredComponent,
		FBoundsAnalysis& OutAnalysis);

	/** Method 6: inspect current physics state. This is a snapshot, not historical motion tracking. */
	MYPROJECT2_API bool QueryPhysicsState(const AActor* Actor, const UPrimitiveComponent* PreferredComponent,
		FPhysicsStateAnalysis& OutAnalysis);

	/** Stage 2: normalize a successful line-trace hit. */
	MYPROJECT2_API bool AnalyzeHit(const FScanResult& Scan, FWorldAnalysis& OutAnalysis);
	MYPROJECT2_API EObjectType ClassifyObject(const FHitResult& Hit);
	MYPROJECT2_API ESurfaceProfile ClassifySurface(const FVector& WorldNormal, float* OutAngleDegrees = nullptr);
	MYPROJECT2_API EPhysicalState ClassifyPhysicalState(const FHitResult& Hit);
	MYPROJECT2_API ECapability InferCapabilities(EObjectType ObjectType, EPhysicalState PhysicalState);
	MYPROJECT2_API bool HasWorldTag(const AActor* Actor, FName Tag);
}
