// Copyright Epic Games, Inc. All Rights Reserved.

#include "FireStone.h"
#include "Campfire.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "StonePhaseF_Environment/StoneShapeRecognitionSubsystem.h"

AFireStone::AFireStone()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Greybox/Meshes/SM_GB_Stone.SM_GB_Stone")));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

AFireStone* AFireStone::Place(UWorld* World, const FVector& Ground)
{
	return World ? World->SpawnActor<AFireStone>(AFireStone::StaticClass(), FTransform(FRotator(0.f, FMath::FRand() * 360.f, 0.f), Ground + FVector(0.f, 0.f, 9.f))) : nullptr;
}

int32 AFireStone::CountNear(UWorld* World, const FVector& Where)
{
	int32 Count = 0;
	for (TActorIterator<AFireStone> It(World); It; ++It)
	{
		Count += FVector::Dist2D(It->GetActorLocation(), Where) <= RingReachCm ? 1 : 0;
	}
	return Count;
}

ACampfire* AFireStone::TryForm(UWorld* World, const AFireStone* New)
{
	if (!World || !New)
	{
		return nullptr;
	}
	TArray<AFireStone*> Ring;
	for (TActorIterator<AFireStone> It(World); It; ++It)
	{
		if (FVector::Dist2D(It->GetActorLocation(), New->GetActorLocation()) <= RingReachCm)
		{
			Ring.Add(*It);
		}
	}
	if (Ring.Num() < StonesPerPit)
	{
		return nullptr;
	}
	// The seven nearest to the newest stone make the ring.
	Ring.Sort([New](const AFireStone& A, const AFireStone& B)
	{
		return FVector::DistSquared2D(A.GetActorLocation(), New->GetActorLocation()) < FVector::DistSquared2D(B.GetActorLocation(), New->GetActorLocation());
	});
	Ring.SetNum(StonesPerPit);
	TArray<FVector> GroundPoints;
	GroundPoints.Reserve(Ring.Num());
	for (const AFireStone* Stone : Ring)
	{
		// FireStone actors are placed nine centimetres above the contact plane.
		GroundPoints.Add(Stone->GetActorLocation() - FVector(0.f, 0.f, 9.f));
	}
	FVector Middle;
	float RadiusCm = 0.f, RadialErrorCm = 0.f, MaxGapDegrees = 360.f;
	if (!UStoneShapeRecognitionSubsystem::EvaluateHearthRing(
		GroundPoints, Middle, RadiusCm, RadialErrorCm, MaxGapDegrees))
	{
		// Seven nearby stones are not enough: they must actually enclose a hearth.
		return nullptr;
	}
	if (FVector::Dist2D(New->GetActorLocation(), Middle) > RingReachCm)
	{
		return nullptr;
	}
	for (AFireStone* Stone : Ring)
	{
		Stone->Destroy();
	}
	return ACampfire::Build(World, Middle);
}
