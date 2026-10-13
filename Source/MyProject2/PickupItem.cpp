// Copyright Epic Games, Inc. All Rights Reserved.

#include "PickupItem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"

APickupItem::APickupItem()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	// While falling the player passes through so physics and movement do not fight.
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Mesh->SetGenerateOverlapEvents(false);
	RootComponent = Mesh;
}

APickupItem* APickupItem::Spawn(UWorld* World, EForestItem Item, int32 Count, const FTransform& Transform, const FVector& Impulse)
{
	UStaticMesh* ItemMesh = ForestItems::GetMesh(Item, FMath::Rand());
	if (!World || !ItemMesh)
	{
		return nullptr;
	}
	FTransform Scaled = Transform;
	Scaled.SetScale3D(Transform.GetScale3D() * ForestItems::GetItemScale(Item));
	APickupItem* Pickup = World->SpawnActor<APickupItem>(APickupItem::StaticClass(), Scaled);
	if (Pickup)
	{
		Pickup->Init(Item, Count, ItemMesh, false, Impulse);
	}
	return Pickup;
}

APickupItem* APickupItem::SpawnCosmetic(UWorld* World, UStaticMesh* InMesh, const FTransform& Transform, const FVector& Impulse)
{
	if (!World || !InMesh)
	{
		return nullptr;
	}
	APickupItem* Pickup = World->SpawnActor<APickupItem>(APickupItem::StaticClass(), Transform);
	if (Pickup)
	{
		Pickup->Init(EForestItem::None, 0, InMesh, true, Impulse);
	}
	return Pickup;
}

void APickupItem::Init(EForestItem InItem, int32 InCount, UStaticMesh* InMesh, bool bInCosmetic, const FVector& Impulse)
{
	Item = InItem;
	Count = InCount;
	bCosmetic = bInCosmetic;

	Mesh->SetStaticMesh(InMesh);
	if (bCosmetic)
	{
		Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Mesh->SetCastShadow(false);
		SetLifeSpan(5.f);
	}
	Mesh->SetSimulatePhysics(true);
	if (!Impulse.IsNearlyZero())
	{
		Mesh->AddImpulse(Impulse, NAME_None, true);
	}
}

void APickupItem::BeginCarry(USceneComponent* AttachTo, const FTransform& Relative)
{
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	SetActorTickEnabled(false);
	AttachToComponent(AttachTo, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Mesh->SetRelativeTransform(FTransform(Relative.GetRotation(), Relative.GetLocation(), Mesh->GetRelativeScale3D()));
}

void APickupItem::Drop(const FTransform& Transform)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	const FVector Scale = Mesh->GetComponentScale();
	Mesh->SetRelativeTransform(FTransform::Identity);
	SetActorTransform(FTransform(Transform.GetRotation(), Transform.GetLocation(), Scale));
	Mesh->SetCastShadow(true);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetSimulatePhysics(true);
	Age = 0.f;
	SetActorTickEnabled(true);
}

void APickupItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	// Nothing an axe or a drop produces moves faster than this; a piece that spawns overlapping
	// something can otherwise be thrown across the map by the physics pushing it out.
	constexpr float MaxSpeed = 1200.f;
	const FVector Velocity = Mesh->GetPhysicsLinearVelocity();
	if (Velocity.SizeSquared() > MaxSpeed * MaxSpeed)
	{
		Mesh->SetPhysicsLinearVelocity(Velocity.GetClampedToMaxSize(MaxSpeed));
	}
	const bool bResting = Age > 2.f && !Mesh->IsAnyRigidBodyAwake();
	if (bResting || Age > 12.f)
	{
		if (!bCosmetic)
		{
			// Once settled the item becomes solid: the player can stand on logs and cannot walk through them.
			Mesh->SetSimulatePhysics(false);
			Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
		SetActorTickEnabled(false);
	}
}
