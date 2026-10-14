// Copyright Epic Games, Inc. All Rights Reserved.

#include "HarvestableActor.h"
#include "PickupItem.h"
#include "CarvableLog.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

AHarvestableActor::AHarvestableActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Mesh;
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
			if (!HitActor || (!HitActor->IsA<AHarvestableActor>() && !HitActor->IsA<APickupItem>()))
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

	// Find the ground itself: skip standing trees, rocks, other logs and items so a log
	// falling against a neighbouring tree does not end up resting on its canopy.
	FVector Grounded = Center;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LogGround), false, Ignore);
	TArray<FHitResult> Hits;
	if (World && World->LineTraceMultiByObjectType(Hits, Center + FVector(0, 0, 800.f), Center - FVector(0, 0, 1500.f), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		for (const FHitResult& Ground : Hits)
		{
			const AActor* HitActor = Ground.GetActor();
			if (!HitActor || (!HitActor->IsA<AHarvestableActor>() && !HitActor->IsA<APickupItem>()))
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
	return IsTree() ? TEXT("Ağaç") : TEXT("Kaya");
}

bool AHarvestableActor::ApplyHit(float Damage, const FHitResult& Hit, const FVector& HitDirection)
{
	if (!IsAlive() || bFalling)
	{
		return false;
	}

	Health = FMath::Max(0.f, Health - Damage);
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
		const float Alpha = FMath::Clamp(FallTime / FallDuration, 0.f, 1.f);
		// Slow start, fast finish, like a real tree going over.
		const float Angle = FMath::DegreesToRadians(88.f) * Alpha * Alpha;
		SetActorRotation(FQuat(FallAxis, Angle) * StartRotation);
		if (Alpha >= 1.f)
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

	bFalling = true;
	FallTime = 0.f;
	SetActorTickEnabled(true);
}

int32 AHarvestableActor::LogsFromHeight(float TreeHeightCm)
{
	return FMath::Clamp(FMath::FloorToInt(TreeHeightCm * 0.85f / (6.f * ACarvableLog::ModuleLength)), 1, 4);
}

void AHarvestableActor::FinishFall()
{
	UWorld* World = GetWorld();

	// The tree leaves its trunk (stage 1 log) lying along the fall line.
	if (const UStaticMesh* TrunkMesh = ForestItems::GetLogStageMesh(1))
	{
		const float Length = TrunkMesh->GetBoundingBox().GetSize().X * ForestItems::GetLogStageScale(1).X;
		const FVector Center = BaseLocation + FallDirection * (100.f + Length * 0.5f);
		if (AHarvestableActor* Trunk = World->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), FTransform(Center)))
		{
			Trunk->InitLog(1, Center, FallDirection);
			Trunk->LogCount = LogsFromHeight(Height);
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

	for (int32 Index = 0; Index < DropCount; ++Index)
	{
		const FVector Offset = FMath::VRand() * Bounds.BoxExtent * 0.4f;
		const FVector Location = Bounds.Origin + FVector(Offset.X, Offset.Y, FMath::Abs(Offset.Z));
		const FVector Impulse = FVector(Offset.X, Offset.Y, 0.f).GetSafeNormal() * 180.f + FVector(0, 0, 250.f);
		APickupItem::Spawn(World, EForestItem::Stone, 1, FTransform(FRotator(0, FMath::FRand() * 360.f, 0), Location), Impulse);
	}
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
