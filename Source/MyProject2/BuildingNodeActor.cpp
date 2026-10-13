// Copyright Epic Games, Inc. All Rights Reserved.

#include "BuildingNodeActor.h"
#include "StructuralIntegritySubsystem.h"
#include "Engine/World.h"

ABuildingNodeActor::ABuildingNodeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SelfWeight = 150.0f;           // kg default
	CompressiveCapacityKN = 60.0f; // kN default
	CurrentLoadKN = 0.0f;
	StructuralHealth = 150.0f;
	bIsGrounded = false;
	bIsCollapsed = false;
}

void ABuildingNodeActor::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UStructuralIntegritySubsystem* Subsystem = World->GetSubsystem<UStructuralIntegritySubsystem>())
		{
			Subsystem->RegisterNode(this);
		}
	}
}

void ABuildingNodeActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UStructuralIntegritySubsystem* Subsystem = World->GetSubsystem<UStructuralIntegritySubsystem>())
		{
			Subsystem->UnregisterNode(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

float ABuildingNodeActor::ApplyDamage(float Amount, const FVector& Direction)
{
	if (bIsCollapsed)
	{
		return 0.0f;
	}

	StructuralHealth = FMath::Clamp(StructuralHealth - Amount, 0.0f, 1000.0f);

	if (StructuralHealth <= 0.0f)
	{
		const FVector Impulse = Direction.GetSafeNormal() * 400.0f + FVector(0.0f, 0.0f, 80.0f);
		TriggerChaosCollapse(Impulse);
	}

	return StructuralHealth;
}

void ABuildingNodeActor::TriggerChaosCollapse(const FVector& Impulse)
{
	if (bIsCollapsed)
	{
		return;
	}

	bIsCollapsed = true;

	if (UWorld* World = GetWorld())
	{
		if (UStructuralIntegritySubsystem* Subsystem = World->GetSubsystem<UStructuralIntegritySubsystem>())
		{
			Subsystem->TriggerCascadeFailure(this, Impulse);
		}
	}
}

void ABuildingNodeActor::AddSupportingNode(ABuildingNodeActor* Child)
{
	if (Child && !SupportingNodes.Contains(Child))
	{
		SupportingNodes.Add(Child);
		Child->AddSupportedByNode(this);
	}
}

void ABuildingNodeActor::RemoveSupportingNode(ABuildingNodeActor* Child)
{
	if (Child)
	{
		SupportingNodes.Remove(Child);
		Child->SupportedByNodes.Remove(this);
	}
}

void ABuildingNodeActor::AddSupportedByNode(ABuildingNodeActor* Parent)
{
	if (Parent && !SupportedByNodes.Contains(Parent))
	{
		SupportedByNodes.Add(Parent);
	}
}
