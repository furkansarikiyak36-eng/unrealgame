// Copyright Epic Games, Inc. All Rights Reserved.

#include "StructureManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"

AStructureManager::AStructureManager()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

UInstancedStaticMeshComponent* AStructureManager::GetOrCreatePieces(EForestItem Item)
{
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = Pieces.Find(Item))
	{
		return *Found;
	}

	UStaticMesh* Mesh = ForestItems::GetMesh(Item, 0);
	if (!Mesh)
	{
		return nullptr;
	}

	UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(this);
	Instances->SetMobility(EComponentMobility::Movable);
	Instances->SetStaticMesh(Mesh);
	Instances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Instances->SetupAttachment(Root);
	Instances->RegisterComponent();
	AddInstanceComponent(Instances);

	Pieces.Add(Item, Instances);
	return Instances;
}

const UInstancedStaticMeshComponent* AStructureManager::FindPieces(EForestItem Item) const
{
	const TObjectPtr<UInstancedStaticMeshComponent>* Found = Pieces.Find(Item);
	return Found ? Found->Get() : nullptr;
}

void AStructureManager::AddPiece(EForestItem Item, const FTransform& WorldTransform)
{
	if (UInstancedStaticMeshComponent* Instances = GetOrCreatePieces(Item))
	{
		Instances->AddInstance(WorldTransform, /*bWorldSpace*/ true);
	}
}
