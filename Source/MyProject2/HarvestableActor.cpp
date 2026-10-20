// Copyright Epic Games, Inc. All Rights Reserved.

#include "HarvestableActor.h"
#include "PickupItem.h"
#include "CarvableLog.h"
#include "LooseResources.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Volume.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "UObject/Package.h"
#include "TimerManager.h"

AHarvestableActor::AHarvestableActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Mesh;

	// Unused (empty, no collision) unless InitFromSkeletal fills them.
	TreeSkin = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TreeSkin"));
	TreeSkin->SetupAttachment(Mesh);
	TreeSkin->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TrunkCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TrunkCollider"));
	TrunkCollider->SetupAttachment(Mesh);
	TrunkCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AHarvestableActor::InitFromSkeletal(const USkeletalMeshComponent* Source)
{
	HarvestType = EHarvestType::Tree;
	Mesh->SetStaticMesh(nullptr);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TreeSkin->SetSkeletalMesh(Source->GetSkeletalMeshAsset());
	for (int32 Index = 0; Index < Source->GetNumMaterials(); ++Index)
	{
		TreeSkin->SetMaterial(Index, Source->GetMaterial(Index));
	}
	TreeSkin->SetCastShadow(Source->CastShadow);

	const FVector Scale = GetActorScale3D();
	const FBoxSphereBounds Bounds = Source->GetSkeletalMeshAsset()->GetBounds();
	Height = (Bounds.Origin.Z + Bounds.BoxExtent.Z) * Scale.Z;
	SetupTrunkCollider();

	bSimpleTree = true;
	MaxHealth = Height > 1500.f ? 160.f : 120.f;
	Health = MaxHealth;
	BaseLocation = GetActorLocation();
}

void AHarvestableActor::InitFromStaticTree(const UStaticMeshComponent* Source)
{
	HarvestType = EHarvestType::Tree;
	Mesh->SetStaticMesh(Source->GetStaticMesh());
	for (int32 Index = 0; Index < Source->GetNumMaterials(); ++Index)
	{
		Mesh->SetMaterial(Index, Source->GetMaterial(Index));
	}
	Mesh->SetCastShadow(Source->CastShadow);
	// The canopy must not stop the player or the axe; the trunk capsule does that.
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	const FBoxSphereBounds Bounds = Source->GetStaticMesh()->GetBounds();
	Height = (Bounds.Origin.Z + Bounds.BoxExtent.Z) * GetActorScale3D().Z;
	SetupTrunkCollider();

	bSimpleTree = true;
	MaxHealth = Height > 1500.f ? 160.f : 120.f;
	Health = MaxHealth;
	BaseLocation = GetActorLocation();
}

namespace
{
// Megascans stone meshes of the stone chain (all have one convex hull, so pieces can fly with physics).
const TCHAR* const MediumStoneMeshes[] = {
	TEXT("/Game/Fab/Megascans/3D/Beach_Boulder_wenobdn/Low/wenobdn_tier_3/StaticMeshes/wenobdn_tier_3.wenobdn_tier_3"),
	TEXT("/Game/Fab/Megascans/3D/Nordic_Beach_Rock_vbumba2fa/Low/vbumba2fa_tier_3/StaticMeshes/vbumba2fa_tier_3.vbumba2fa_tier_3"),
	TEXT("/Game/Fab/Megascans/3D/Sandstone_Boulder_vmjjfdm/Low/vmjjfdm_tier_3/StaticMeshes/vmjjfdm_tier_3.vmjjfdm_tier_3"),
};
const TCHAR* const SmallStoneMeshes[] = {
	TEXT("/Game/Fab/Megascans/3D/Mossy_Stones_Pack_tliiadmva/Low/tliiadmva_tier_3/StaticMeshes/tliiadmva_LOD0_TIER3_000.tliiadmva_LOD0_TIER3_000"),
	TEXT("/Game/Fab/Megascans/3D/Mossy_Stones_Pack_tliiadmva/Low/tliiadmva_tier_3/StaticMeshes/tliiadmva_LOD0_TIER3_001.tliiadmva_LOD0_TIER3_001"),
	TEXT("/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_000.vcljbb1iw_LOD0_TIER3_000"),
	TEXT("/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_001.vcljbb1iw_LOD0_TIER3_001"),
	TEXT("/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_002.vcljbb1iw_LOD0_TIER3_002"),
	TEXT("/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_003.vcljbb1iw_LOD0_TIER3_003"),
	TEXT("/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_004.vcljbb1iw_LOD0_TIER3_004"),
	TEXT("/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_005.vcljbb1iw_LOD0_TIER3_005"),
	TEXT("/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_006.vcljbb1iw_LOD0_TIER3_006"),
	TEXT("/Game/Fab/Megascans/3D/Small_Stones_Pack_vdjkbfmiw/Low/vdjkbfmiw_tier_3/StaticMeshes/vdjkbfmiw_LOD0_TIER3_001.vdjkbfmiw_LOD0_TIER3_001"),
	TEXT("/Game/Fab/Megascans/3D/Small_Stones_Pack_vdjkbfmiw/Low/vdjkbfmiw_tier_3/StaticMeshes/vdjkbfmiw_LOD0_TIER3_002.vdjkbfmiw_LOD0_TIER3_002"),
};
const TCHAR* const GrayCobbleMeshes[] = {
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx/Low/xksldamqx_tier_3/StaticMeshes/xksldamqx_LOD0_TIER3_000.xksldamqx_LOD0_TIER3_000"),
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx/Low/xksldamqx_tier_3/StaticMeshes/xksldamqx_LOD0_TIER3_001.xksldamqx_LOD0_TIER3_001"),
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx/Low/xksldamqx_tier_3/StaticMeshes/xksldamqx_LOD0_TIER3_002.xksldamqx_LOD0_TIER3_002"),
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx/Low/xksldamqx_tier_3/StaticMeshes/xksldamqx_LOD0_TIER3_003.xksldamqx_LOD0_TIER3_003"),
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx/Low/xksldamqx_tier_3/StaticMeshes/xksldamqx_LOD0_TIER3_004.xksldamqx_LOD0_TIER3_004"),
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx/Low/xksldamqx_tier_3/StaticMeshes/xksldamqx_LOD0_TIER3_005.xksldamqx_LOD0_TIER3_005"),
};
const TCHAR* const BrownCobbleMeshes[] = {
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx/Low/xkqmdhaqx_tier_3/StaticMeshes/xkqmdhaqx_LOD0_TIER3_000.xkqmdhaqx_LOD0_TIER3_000"),
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx/Low/xkqmdhaqx_tier_3/StaticMeshes/xkqmdhaqx_LOD0_TIER3_001.xkqmdhaqx_LOD0_TIER3_001"),
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx/Low/xkqmdhaqx_tier_3/StaticMeshes/xkqmdhaqx_LOD0_TIER3_002.xkqmdhaqx_LOD0_TIER3_002"),
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx/Low/xkqmdhaqx_tier_3/StaticMeshes/xkqmdhaqx_LOD0_TIER3_003.xkqmdhaqx_LOD0_TIER3_003"),
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx/Low/xkqmdhaqx_tier_3/StaticMeshes/xkqmdhaqx_LOD0_TIER3_004.xkqmdhaqx_LOD0_TIER3_004"),
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx/Low/xkqmdhaqx_tier_3/StaticMeshes/xkqmdhaqx_LOD0_TIER3_005.xkqmdhaqx_LOD0_TIER3_005"),
	TEXT("/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx/Low/xkqmdhaqx_tier_3/StaticMeshes/xkqmdhaqx_LOD0_TIER3_006.xkqmdhaqx_LOD0_TIER3_006"),
};
const TCHAR* const WallStoneMeshes[] = {
	TEXT("/Game/Fab/Megascans/3D/Castle_Wall_Stone_sdAta/Low/sdAta_tier_3/StaticMeshes/sdAta_tier_3.sdAta_tier_3"),
};

template <int32 N>
UStaticMesh* PickStoneMesh(const TCHAR* const (&Paths)[N])
{
	return LoadObject<UStaticMesh>(nullptr, Paths[FMath::RandRange(0, N - 1)]);
}

float LargestSide(const UStaticMesh* StoneMesh)
{
	return StoneMesh ? StoneMesh->GetBoundingBox().GetSize().GetMax() : 0.f;
}
}

void AHarvestableActor::InitFromStaticRock(const UStaticMeshComponent* Source)
{
	HarvestType = EHarvestType::Rock;
	Mesh->SetStaticMesh(Source->GetStaticMesh());
	for (int32 Index = 0; Index < Source->GetNumMaterials(); ++Index)
	{
		Mesh->SetMaterial(Index, Source->GetMaterial(Index));
	}
	Mesh->SetCastShadow(Source->CastShadow);
	const float Size = LargestSide(Source->GetStaticMesh()) * GetActorScale3D().GetMax();
	StoneTier = Size >= 100.f ? 3 : Size >= 40.f ? 2 : 1;
	MaxHealth = Health = StoneTier == 3 ? 120.f : StoneTier == 2 ? 60.f : 1.f;
	BaseLocation = GetActorLocation();
}

void AHarvestableActor::InitLooseStone(int32 Tier, UStaticMesh* StoneMesh, float Scale, const FVector& Impulse)
{
	HarvestType = EHarvestType::Rock;
	StoneTier = Tier;
	Mesh->SetStaticMesh(StoneMesh);
	SetActorScale3D(FVector(Scale));
	MaxHealth = Health = Tier >= 2 ? 60.f : 1.f;

	// Thrown out by the break, then, once settled, a still piece of the ground again (no physics cost).
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Mesh->SetSimulatePhysics(true);
	Mesh->AddImpulse(Impulse, NAME_None, true);
	Mesh->AddAngularImpulseInDegrees(FMath::VRand() * 360.f, NAME_None, true);
	FTimerHandle Settle;
	TWeakObjectPtr<AHarvestableActor> Self(this);
	GetWorldTimerManager().SetTimer(Settle, [Self]()
	{
		if (AHarvestableActor* Stone = Self.Get())
		{
			Stone->Mesh->SetSimulatePhysics(false);
			Stone->Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Stone->BaseLocation = Stone->GetActorLocation();
		}
	}, 4.f, false);
}

void AHarvestableActor::BreakStone()
{
	UWorld* World = GetWorld();
	const FBoxSphereBounds Bounds = Mesh->Bounds;
	PlayFellingSound(World, TEXT("SW_RockBreak"), Bounds.Origin);

	// Large -> three medium stones, medium -> four small ones, thrown outwards and up.
	const int32 PieceTier = StoneTier - 1;
	const int32 Count = StoneTier >= 3 ? 3 : 4;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = 2.f * PI * Index / Count + FMath::FRand() * 0.6f;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		UStaticMesh* PieceMesh = PieceTier >= 2 ? PickStoneMesh(MediumStoneMeshes) : PickStoneMesh(SmallStoneMeshes);
		if (!PieceMesh)
		{
			continue;
		}
		// Medium pieces 45-70 cm, small ones 15-30 cm.
		const float Target = PieceTier >= 2 ? FMath::FRandRange(45.f, 70.f) : FMath::FRandRange(15.f, 30.f);
		const float Scale = FMath::Clamp(Target / FMath::Max(1.f, LargestSide(PieceMesh)), 0.3f, 3.f);
		const FVector Location = Bounds.Origin + Out * Bounds.BoxExtent.GetMax() * 0.45f + FVector(0.f, 0.f, Bounds.BoxExtent.Z * 0.3f);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AHarvestableActor* Piece = World->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), FTransform(FMath::VRand().Rotation(), Location), Params))
		{
			const float Speed = PieceTier >= 2 ? FMath::FRandRange(150.f, 280.f) : FMath::FRandRange(250.f, 450.f);
			Piece->InitLooseStone(PieceTier, PieceMesh, Scale, Out * Speed + FVector(0.f, 0.f, FMath::FRandRange(200.f, 380.f)));
		}
	}
	Destroy();
}

bool AHarvestableActor::CarveHit()
{
	if (!IsCarvable() || Mesh->IsSimulatingPhysics())
	{
		return false;
	}
	UWorld* World = GetWorld();
	PlayFellingSound(World, TEXT("SW_StoneCarve"), GetActorLocation());
	if (++CarveHits < CarveHitsNeeded)
	{
		ShakeTime = 0.1f;
		SetActorTickEnabled(true);
		return false;
	}

	// Medium stone -> a flat wall stone about its size; small one -> a cobblestone (brown from 20 cm, else grey).
	const float Size = LargestSide(Mesh->GetStaticMesh()) * GetActorScale3D().GetMax();
	UStaticMesh* Carved = nullptr;
	float Scale = 1.f;
	if (StoneTier >= 2)
	{
		Carved = PickStoneMesh(WallStoneMeshes);
		Scale = FMath::Clamp(Size * 1.1f / FMath::Max(1.f, LargestSide(Carved)), 0.35f, 1.f);
	}
	else if (Size >= 20.f)
	{
		Carved = PickStoneMesh(BrownCobbleMeshes);
		Scale = FMath::Clamp(Size / FMath::Max(1.f, LargestSide(Carved)), 0.7f, 1.3f);
	}
	else
	{
		Carved = PickStoneMesh(GrayCobbleMeshes);
		Scale = FMath::Clamp(Size / FMath::Max(1.f, LargestSide(Carved)), 0.8f, 1.5f);
	}
	if (!Carved)
	{
		return false;
	}

	// Lying flat on the ground where the stone was.
	FVector Location = GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CarvedGround), false, this);
	TArray<FHitResult> Hits;
	if (World->LineTraceMultiByObjectType(Hits, Location + FVector(0, 0, 100.f), Location - FVector(0, 0, 300.f), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		for (const FHitResult& Ground : Hits)
		{
			const AActor* HitActor = Ground.GetActor();
			if (!HitActor || (!HitActor->IsA<AHarvestableActor>() && !HitActor->IsA<APickupItem>() && !HitActor->IsA<AVolume>()))
			{
				Location.Z = Ground.ImpactPoint.Z - Carved->GetBoundingBox().Min.Z * Scale;
				break;
			}
		}
	}
	Mesh->SetStaticMesh(Carved);
	for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
	{
		Mesh->SetMaterial(Index, nullptr);
	}
	SetActorTransform(FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), Location, FVector(Scale)));
	StoneTier = -1;
	CarveHits = 0;
	MaxHealth = Health = 1.f;
	BaseLocation = GetActorLocation();
	return true;
}

void AHarvestableActor::SetupTrunkCollider()
{
	// Trunk only: the lower 40 % of the tree, thicker for tall trees. Blocks pawns and the axe trace.
	// Capsule sizes are local, so divide out the actor scale.
	const FVector Scale = GetActorScale3D();
	const float ScaleXY = FMath::Max(FMath::Max(Scale.X, Scale.Y), KINDA_SMALL_NUMBER);
	const float ScaleZ = FMath::Max(Scale.Z, KINDA_SMALL_NUMBER);
	TrunkRadius = Height > 1500.f ? 35.f : 25.f;
	const float Radius = TrunkRadius / ScaleXY;
	const float HalfHeight = FMath::Max(Radius, Height * 0.2f / ScaleZ);
	TrunkCollider->InitCapsuleSize(Radius, HalfHeight);
	TrunkCollider->SetRelativeLocation(FVector(0.f, 0.f, HalfHeight));
	TrunkCollider->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

UStaticMesh* AHarvestableActor::LoadRawLog(int32 LengthCm)
{
	if (UStaticMesh* Exact = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/UnrealExport/SM_RawLog_%d.SM_RawLog_%d"), LengthCm, LengthCm)))
	{
		return Exact;
	}
	// A length without its own mesh: the 150 cm one, scaled along its length by InitRawLog.
	return LoadObject<UStaticMesh>(nullptr, TEXT("/Game/UnrealExport/SM_RawLog_150.SM_RawLog_150"));
}

ACarvableLog* AHarvestableActor::ConvertToCarvable()
{
	UStaticMesh* LogMesh = RawLogLength > 0 ? Mesh->GetStaticMesh() : nullptr;
	if (!LogMesh)
	{
		return nullptr;
	}
	ACarvableLog* Piece = ACarvableLog::SpawnFromStaticMesh(GetWorld(), LogMesh, GetActorTransform());
	if (Piece)
	{
		Destroy();
	}
	return Piece;
}

TArray<int32> AHarvestableActor::RawLogsForHeight(float TreeHeightCm)
{
	if (TreeHeightCm >= 1800.f)
	{
		return { 600, 300 };
	}
	return { TreeHeightCm >= 1000.f ? 600 : 300 };
}

void AHarvestableActor::PlayFellingSound(UWorld* World, const TCHAR* Name, const FVector& Location, float Volume)
{
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Audio/Felling/%s.%s"), Name, Name));
	if (!World || !Sound)
	{
		return;
	}
	// One shared falloff: full volume within 4 m, silent beyond 45 m.
	static USoundAttenuation* Falloff = nullptr;
	if (!Falloff)
	{
		Falloff = NewObject<USoundAttenuation>(GetTransientPackage(), TEXT("FellingSoundFalloff"));
		Falloff->AddToRoot();
		Falloff->Attenuation.bAttenuate = true;
		Falloff->Attenuation.bSpatialize = true;
		Falloff->Attenuation.AttenuationShapeExtents = FVector(400.f, 0.f, 0.f);
		Falloff->Attenuation.FalloffDistance = 4100.f;
	}
	UGameplayStatics::PlaySoundAtLocation(World, Sound, Location, Volume, 1.f, 0.f, Falloff);
}

void AHarvestableActor::InitRawLog(int32 LengthCm, const FVector& Center, const FVector& Direction)
{
	HarvestType = EHarvestType::Trunk;
	RawLogLength = LengthCm;
	LogStage = LengthCm >= 600 ? 1 : 2;
	UStaticMesh* LogMesh = LoadRawLog(LengthCm);
	Mesh->SetStaticMesh(LogMesh);
	if (LogMesh)
	{
		const float MeshLength = LogMesh->GetBoundingBox().GetSize().X;
		const FVector LogScale(MeshLength > 1.f ? FMath::Clamp(LengthCm / MeshLength, 0.25f, 1.f) : 1.f, 1.f, 1.f);
		SetActorTransform(GroundedLogTransform(GetWorld(), LogMesh, LogScale, Center, Direction, this));
	}
	// Longer logs take a few more swings.
	MaxHealth = Health = LengthCm >= 600 ? 80.f : LengthCm >= 300 ? 60.f : LengthCm >= 150 ? 40.f : 30.f;
	MaxHitMarks = 10;
	BaseLocation = GetActorLocation();
}

void AHarvestableActor::InitFrom(const UStaticMeshComponent* Source, EHarvestType InType)
{
	HarvestType = InType;
	Mesh->SetStaticMesh(Source->GetStaticMesh());
	for (int32 Index = 0; Index < Source->GetNumMaterials(); ++Index)
	{
		Mesh->SetMaterial(Index, Source->GetMaterial(Index));
	}

	const FVector Scale = GetActorScale3D();
	const FBoxSphereBounds Bounds = Mesh->GetStaticMesh()->GetBounds();
	Height = Bounds.BoxExtent.Z * 2.f * Scale.Z;

	if (IsTree())
	{
		MaxHealth = Height > 1500.f ? 160.f : 120.f;
	}
	else
	{
		const float Size = Scale.GetMax();
		MaxHealth = 60.f + 40.f * Size;
		DropCount = FMath::Clamp(FMath::RoundToInt(4.f * Size) + 2, 3, 10);
	}
	Health = MaxHealth;
	BaseLocation = GetActorLocation();
}

void AHarvestableActor::InitLog(int32 InStage, const FVector& Center, const FVector& Direction)
{
	HarvestType = EHarvestType::Trunk;
	LogStage = InStage;

	UStaticMesh* LogMesh = ForestItems::GetLogStageMesh(InStage);
	Mesh->SetStaticMesh(LogMesh);
	if (LogMesh)
	{
		SetActorTransform(GroundedLogTransform(GetWorld(), LogMesh, ForestItems::GetLogStageScale(InStage), Center, Direction, this));
	}

	// The trunk takes a few more swings than its halves.
	MaxHealth = Health = InStage == 1 ? 80.f : 60.f;
	MaxHitMarks = 10;
	BaseLocation = GetActorLocation();
}

void AHarvestableActor::BeginDrag(const FVector& Holder)
{
	const UStaticMesh* LogMesh = Mesh->GetStaticMesh();
	if (!LogMesh)
	{
		return;
	}
	const FBox Box = LogMesh->GetBoundingBox();
	const FVector Scale = GetActorScale3D();
	DragLength = Box.GetSize().X * Scale.X;
	DragRadius = Box.GetExtent().Z * Scale.Z;

	const FVector Center = GetActorTransform().TransformPosition(Box.GetCenter());
	FVector Axis = GetActorForwardVector();
	Axis.Z = 0.f;
	Axis.Normalize();
	const FVector EndA = Center + Axis * DragLength * 0.5f;
	const FVector EndB = Center - Axis * DragLength * 0.5f;
	DragBack = FVector::DistSquared2D(EndA, Holder) < FVector::DistSquared2D(EndB, Holder) ? EndB : EndA;

	bDragged = true;
	ShakeTime = 0.f;
	SetActorEnableCollision(false);
}

void AHarvestableActor::DragTo(const FVector& Front)
{
	const UStaticMesh* LogMesh = Mesh->GetStaticMesh();
	if (!bDragged || !LogMesh)
	{
		return;
	}
	// Trailing end: keep the log's length behind the lifted end and rest it on the ground.
	FVector Dir = Front - DragBack;
	Dir.Z = 0.f;
	if (!Dir.Normalize())
	{
		Dir = GetActorForwardVector();
	}
	FVector Back = Front - Dir * DragLength;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LogDrag), false, this);
	TArray<FHitResult> Hits;
	if (GetWorld()->LineTraceMultiByObjectType(Hits, Back + FVector(0, 0, 300.f), Back - FVector(0, 0, 600.f), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		for (const FHitResult& Ground : Hits)
		{
			const AActor* HitActor = Ground.GetActor();
			if (!HitActor || (!HitActor->IsA<AHarvestableActor>() && !HitActor->IsA<APickupItem>() && !HitActor->IsA<AVolume>()))
			{
				Back.Z = Ground.ImpactPoint.Z + DragRadius;
				break;
			}
		}
	}
	DragBack = Back;

	const FVector Center = (Front + Back) * 0.5f;
	const FQuat Rotation = (Front - Back).Rotation().Quaternion();
	const FBox Box = LogMesh->GetBoundingBox();
	SetActorTransform(FTransform(Rotation, Center - Rotation.RotateVector(Box.GetCenter() * GetActorScale3D()), GetActorScale3D()));
}

void AHarvestableActor::BeginShoulder(USceneComponent* AttachTo, const FTransform& Relative, bool bCastShadow)
{
	ShakeTime = 0.f;
	SetActorTickEnabled(false);
	SetActorEnableCollision(false);
	Mesh->SetCastShadow(bCastShadow);
	const FVector Scale = GetActorScale3D();
	AttachToComponent(AttachTo, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeTransform(FTransform(Relative.GetRotation(), Relative.GetLocation(), Scale));
}

void AHarvestableActor::PlaceOnGround(const FVector& Center, const FVector& Direction)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Mesh->SetCastShadow(true);
	SetActorEnableCollision(true);
	if (const UStaticMesh* LogMesh = Mesh->GetStaticMesh())
	{
		SetActorTransform(GroundedLogTransform(GetWorld(), LogMesh, GetActorScale3D(), Center, Direction, this));
	}
	BaseLocation = GetActorLocation();
}

void AHarvestableActor::EndDrag()
{
	const UStaticMesh* LogMesh = Mesh->GetStaticMesh();
	bDragged = false;
	SetActorEnableCollision(true);
	if (LogMesh)
	{
		const FVector Center = GetActorTransform().TransformPosition(LogMesh->GetBoundingBox().GetCenter());
		SetActorTransform(GroundedLogTransform(GetWorld(), LogMesh, GetActorScale3D(), Center, GetActorForwardVector(), this));
	}
	BaseLocation = GetActorLocation();
}

FTransform AHarvestableActor::GroundedLogTransform(UWorld* World, const UStaticMesh* LogMesh, const FVector& Scale, const FVector& Center, const FVector& Direction, const AActor* Ignore)
{
	const FBox Box = LogMesh->GetBoundingBox();
	const FQuat Rotation = FRotator(0.f, Direction.Rotation().Yaw, 0.f).Quaternion();

	// Find the ground itself: skip standing trees, rocks, other logs, items and volumes (a PCG volume's box) so a log
	// falling against a neighbouring tree does not end up resting on its canopy.
	FVector Grounded = Center;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LogGround), false, Ignore);
	TArray<FHitResult> Hits;
	if (World && World->LineTraceMultiByObjectType(Hits, Center + FVector(0, 0, 800.f), Center - FVector(0, 0, 1500.f), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		for (const FHitResult& Ground : Hits)
		{
			const AActor* HitActor = Ground.GetActor();
			if (!HitActor || (!HitActor->IsA<AHarvestableActor>() && !HitActor->IsA<APickupItem>() && !HitActor->IsA<AVolume>()))
			{
				Grounded.Z = Ground.ImpactPoint.Z;
				break;
			}
		}
	}
	Grounded.Z += Box.GetExtent().Z * Scale.Z - 6.f;

	return FTransform(Rotation, Grounded - Rotation.RotateVector(Box.GetCenter() * Scale), Scale);
}

FString AHarvestableActor::GetDisplayName() const
{
	if (IsTrunk())
	{
		return LogStage == 1 ? TEXT("Gövde") : TEXT("Büyük Kütük");
	}
	switch (StoneTier)
	{
	case 3: return TEXT("Kaya");
	case 2: return TEXT("Taş");
	case 1: return TEXT("Küçük Taş");
	case -1: return TEXT("Kesme Taş");
	default: break;
	}
	return IsTree() ? TEXT("Ağaç") : TEXT("Kaya");
}

bool AHarvestableActor::ApplyHit(float Damage, const FHitResult& Hit, const FVector& HitDirection)
{
	if (!IsAlive() || bFalling)
	{
		return false;
	}

	// Breakable rocks and their pieces: the pickaxe is heard; small and carved stones stay whole.
	if (StoneTier != 0)
	{
		PlayFellingSound(GetWorld(), TEXT("SW_PickaxeHit"), Hit.ImpactPoint);
		if (StoneTier < 2)
		{
			return false;
		}
		Health = FMath::Max(0.f, Health - Damage);
		if (Health <= 0.f)
		{
			BreakStone();
			return true;
		}
		if (!Mesh->IsSimulatingPhysics())
		{
			ShakeTime = 0.12f;
			SetActorTickEnabled(true);
		}
		return false;
	}

	Health = FMath::Max(0.f, Health - Damage);
	if (IsWood())
	{
		PlayFellingSound(GetWorld(), TEXT("SW_AxeChop"), Hit.ImpactPoint);
	}
	// Simple trees: the hit is only heard.
	if (bSimpleTree && IsTree())
	{
		if (Health <= 0.f)
		{
			StartFall(HitDirection);
			return true;
		}
		return false;
	}
	SpawnDebris(Hit.ImpactPoint, Hit.ImpactNormal, 2);
	AddHitMark(Hit);

	if (Health <= 0.f)
	{
		if (IsTree())
		{
			StartFall(HitDirection);
		}
		else if (IsTrunk())
		{
			SplitLog();
		}
		else
		{
			BreakRock();
		}
		return true;
	}

	ShakeTime = 0.2f;
	SetActorTickEnabled(true);
	return false;
}

void AHarvestableActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFalling)
	{
		FallTime += DeltaSeconds;
		// Simple trees first break off: a short creaking lean of a few degrees with a tremble, then the fall.
		const float Break = bSimpleTree ? BreakDuration : 0.f;
		const float LeanDeg = bSimpleTree ? 4.f : 0.f;
		float AngleDeg;
		bool bDone = false;
		if (FallTime < Break)
		{
			const float B = FallTime / Break;
			AngleDeg = LeanDeg * B * B + FMath::Sin(FallTime * 55.f) * 0.35f * (1.f - B);
		}
		else
		{
			const float Alpha = FMath::Clamp((FallTime - Break) / FallDuration, 0.f, 1.f);
			// Slow start, fast finish, like a real tree going over.
			AngleDeg = LeanDeg + (88.f - LeanDeg) * Alpha * Alpha;
			bDone = Alpha >= 1.f;
		}
		const FQuat Turn(FallAxis, FMath::DegreesToRadians(AngleDeg));
		SetActorLocationAndRotation(FallPivot + Turn.RotateVector(BaseLocation - FallPivot), Turn * StartRotation);
		if (bDone)
		{
			FinishFall();
		}
		return;
	}

	if (ShakeTime > 0.f)
	{
		ShakeTime -= DeltaSeconds;
		const float Amount = IsWood() ? 3.f : 1.5f;
		SetActorLocation(ShakeTime > 0.f ? BaseLocation + FMath::VRand() * Amount : BaseLocation);
		if (ShakeTime <= 0.f)
		{
			SetActorTickEnabled(false);
		}
	}
}

void AHarvestableActor::StartFall(const FVector& Direction)
{
	FallDirection = FVector(Direction.X, Direction.Y, 0.f).GetSafeNormal();
	if (FallDirection.IsNearlyZero())
	{
		FallDirection = GetActorForwardVector();
	}
	// Rotating around Up x Dir tips the top of the tree towards Dir.
	FallAxis = FVector::CrossProduct(FVector::UpVector, FallDirection);
	StartRotation = GetActorQuat();
	SetActorLocation(BaseLocation);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	TrunkCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	if (bSimpleTree)
	{
		// The trunk gives way: a crack, a stump stays and the tree tips over from the top of the stump.
		PlayFellingSound(GetWorld(), TEXT("SW_TreeCrack"), BaseLocation + FVector(0.f, 0.f, 200.f));
		TrunkCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// The tree breaks off at the stump's top and turns round that point, so it does not jump up.
		const float StumpTop = SpawnStump();
		FallPivot = BaseLocation + FVector(0.f, 0.f, StumpTop * 0.8f);
	}
	else
	{
		FallPivot = BaseLocation;
	}

	bFalling = true;
	FallTime = 0.f;
	SetActorTickEnabled(true);
}

float AHarvestableActor::SpawnStump()
{
	// Megascans Old Tree Stump (about 134 x 88 cm, 57 cm tall).
	static const TCHAR* StumpPath = TEXT("/Game/Fab/Megascans/3D/Old_Tree_Stump_wd0qcaobw/Low/wd0qcaobw_tier_3/StaticMeshes/wd0qcaobw_tier_3.wd0qcaobw_tier_3");
	UStaticMesh* StumpMesh = LoadObject<UStaticMesh>(nullptr, StumpPath);
	UWorld* World = GetWorld();
	if (!StumpMesh || !World)
	{
		return 0.f;
	}
	const FBox Box = StumpMesh->GetBoundingBox();
	const float Width = FMath::Max(1.f, (Box.GetSize().X + Box.GetSize().Y) * 0.5f);
	// About twice the trunk width (the trunk flares at its foot) and never shorter than the meadow grass.
	const float Scale = FMath::Clamp(TrunkRadius * 2.f * 2.2f / Width, 0.9f, 1.3f);
	const FRotator Rotation(0.f, FMath::FRand() * 360.f, 0.f);
	// Bottom of the mesh a few cm into the ground at the tree's base.
	const FVector Location = BaseLocation - FVector(0.f, 0.f, Box.Min.Z * Scale + 4.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Stump = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(Rotation, Location, FVector(Scale)), Params);
	if (!Stump)
	{
		return 0.f;
	}
	Stump->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
	Stump->GetStaticMeshComponent()->SetStaticMesh(StumpMesh);
	Stump->Tags.Add(TEXT("TreeStump"));
	return Box.GetSize().Z * Scale - 4.f;
}

void AHarvestableActor::DropRockResources(UWorld* World, const FVector& Center, int32 Count)
{
	// The stones fall round the rock, a little apart so they do not land on each other.
	for (int32 Index = 0; Index < FMath::Max(1, Count); ++Index)
	{
		const float Angle = 2.f * PI * Index / FMath::Max(1, Count) + FMath::FRand() * 0.4f;
		const FVector Location = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (90.f + 25.f * Index) + FVector(0.f, 0.f, 40.f);
		LooseResources::SpawnStone(World, Location, Index == 0);
	}
}

void AHarvestableActor::DropBranchSticks(UWorld* World, const FVector& Base, const FVector& Direction, float Length)
{
	const FVector Along = FVector(Direction.X, Direction.Y, 0.f).GetSafeNormal();
	const FVector Across = FVector::CrossProduct(FVector::UpVector, Along);
	for (int32 Index = 0; Index < SticksPerTree; ++Index)
	{
		const FVector Location = Base + Along * Length * (0.55f + 0.15f * Index) + Across * (Index % 2 == 0 ? 160.f : -160.f) + FVector(0.f, 0.f, 40.f);
		LooseResources::SpawnStick(World, Location, FRotator(0.f, FMath::FRand() * 360.f, 0.f));
	}
}

int32 AHarvestableActor::LogsFromHeight(float TreeHeightCm)
{
	return FMath::Clamp(FMath::FloorToInt(TreeHeightCm * 0.85f / (6.f * ACarvableLog::ModuleLength)), 1, 4);
}

void AHarvestableActor::FinishFall()
{
	UWorld* World = GetWorld();

	// Simple trees (skeletal Megaplant, level-placed static ones): a thud, then raw UnrealExport logs lie end to end
	// along the fall line where the trunk came down (RawLogsForHeight).
	if (bSimpleTree)
	{
		PlayFellingSound(World, TEXT("SW_TreeFall"), BaseLocation + FallDirection * Height * 0.5f);
		float Offset = 100.f;
		for (const int32 Length : RawLogsForHeight(Height))
		{
			const FVector Center = BaseLocation + FallDirection * (Offset + Length * 0.5f);
			if (AHarvestableActor* Log = World->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), FTransform(Center)))
			{
				Log->InitRawLog(Length, Center, FallDirection);
				Log->LogCount = LogsFromHeight(Height);
			}
			Offset += Length + 30.f;
		}
		DropBranchSticks(World, BaseLocation, FallDirection, Offset);
		Destroy();
		return;
	}

	// The tree leaves its trunk (stage 1 log) lying along the fall line.
	if (const UStaticMesh* TrunkMesh = ForestItems::GetLogStageMesh(1))
	{
		const float Length = TrunkMesh->GetBoundingBox().GetSize().X * ForestItems::GetLogStageScale(1).X;
		const FVector Center = BaseLocation + FallDirection * (100.f + Length * 0.5f);
		if (AHarvestableActor* Trunk = World->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), FTransform(Center)))
		{
			Trunk->InitLog(1, Center, FallDirection);
			Trunk->LogCount = LogsFromHeight(Height);
			DropBranchSticks(World, BaseLocation, FallDirection, Length);
		}
		for (int32 Index = 0; Index < 4; ++Index)
		{
			SpawnDebris(BaseLocation + FallDirection * (Length * (Index + 0.5f) / 4.f) + FVector(0, 0, 80.f), FVector::UpVector, 2);
		}
	}
	Destroy();
}

void AHarvestableActor::SplitLog()
{
	UWorld* World = GetWorld();
	const UStaticMesh* LogMesh = Mesh->GetStaticMesh();
	if (!LogMesh)
	{
		Destroy();
		return;
	}

	// Raw UnrealExport logs: two logs of half the length where the two halves were. The shortest one (75 cm) stays whole.
	if (RawLogLength > 0)
	{
		if (RawLogLength <= ShortestRawLogCm)
		{
			Health = MaxHealth;
			return;
		}
		const int32 HalfLength = RawLogLength / 2;
		FVector Axis = GetActorForwardVector();
		Axis.Z = 0.f;
		Axis.Normalize();
		const FVector Mid = GetActorTransform().TransformPosition(LogMesh->GetBoundingBox().GetCenter());
		for (const float Side : { -1.f, 1.f })
		{
			const FVector HalfCenter = Mid + Axis * Side * (HalfLength * 0.5f + 5.f);
			if (AHarvestableActor* Half = World->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), FTransform(HalfCenter)))
			{
				Half->InitRawLog(HalfLength, HalfCenter, Axis);
			}
		}
		SpawnDebris(Mid, FVector::UpVector, 4);
		Destroy();
		return;
	}

	// Cut in the middle: the two halves land where the two ends of this log were.
	FVector Along = GetActorForwardVector();
	Along.Z = 0.f;
	Along.Normalize();
	const FBox Box = LogMesh->GetBoundingBox();
	const FVector Center = GetActorTransform().TransformPosition(Box.GetCenter());
	const float Quarter = Box.GetSize().X * GetActorScale3D().X * 0.25f;

	for (const float Side : { -1.f, 1.f })
	{
		const FVector HalfCenter = Center + Along * Side * Quarter;
		if (LogStage == 1)
		{
			// The logs inside are shared between the halves; a short tree leaves one half empty.
			const int32 Share = Side < 0.f ? (LogCount + 1) / 2 : LogCount / 2;
			if (Share <= 0)
			{
				continue;
			}
			if (AHarvestableActor* Half = World->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), FTransform(HalfCenter)))
			{
				Half->InitLog(2, HalfCenter, Along);
				Half->LogCount = Share;
			}
		}
		else
		{
			if (LogCount < 2 && Side < 0.f)
			{
				continue;
			}
			// Last split: carryable logs. The two halves exactly fill the big log, so spawned end to end they would
			// touch each other and the next log and the physics would throw one away. They fall apart sideways
			// instead, a little short of the ends.
			const FVector Across = FVector::CrossProduct(FVector::UpVector, Along);
			const FVector LogCenter = Center + Along * Side * (Quarter - 15.f) + Across * Side * 40.f;
			const FRotator Rotation(0.f, Along.Rotation().Yaw, 0.f);
			ACarvableLog::SpawnLog(World, FTransform(Rotation, LogCenter + FVector(0, 0, 15.f)));
		}
	}
	SpawnDebris(Center, FVector::UpVector, 4);
	Destroy();
}

void AHarvestableActor::BreakRock()
{
	UWorld* World = GetWorld();
	const FBoxSphereBounds Bounds = Mesh->Bounds;

	DropRockResources(World, Bounds.Origin, DropCount);
	SpawnDebris(Bounds.Origin, FVector::UpVector, 4);
	Destroy();
}

void AHarvestableActor::AddHitMark(const FHitResult& Hit)
{
	if (HitMarkCount >= MaxHitMarks)
	{
		return;
	}
	UMaterialInterface* Material = ForestItems::GetHitMarkMaterial(HarvestType);
	if (!Material)
	{
		return;
	}

	// Decal X projects into the surface; Y stays horizontal so the cut reads as an axe notch.
	FRotator Rotation = (-Hit.ImpactNormal).Rotation();
	Rotation.Roll = FMath::FRandRange(-12.f, 12.f);
	const FVector Size = IsWood() ? FVector(12.f, 20.f, 20.f) : FVector(10.f, 14.f, 14.f);

	if (UDecalComponent* Decal = UGameplayStatics::SpawnDecalAttached(Material, Size, Mesh, NAME_None, Hit.ImpactPoint, Rotation, EAttachLocation::KeepWorldPosition))
	{
		Decal->SetFadeScreenSize(0.002f);
		++HitMarkCount;
	}
}

void AHarvestableActor::SpawnDebris(const FVector& Location, const FVector& Normal, int32 Num)
{
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const FVector Impulse = Normal * 150.f + FMath::VRand() * 80.f + FVector(0, 0, 150.f);
		const FTransform Transform(FMath::VRand().Rotation(), Location + Normal * 15.f);
		APickupItem::SpawnCosmetic(GetWorld(), ForestItems::GetDebrisMesh(HarvestType, FMath::Rand()), Transform, Impulse);
	}
}
