// Copyright Epic Games, Inc. All Rights Reserved.

#include "LooseResources.h"
#include "ForestTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "StickCore/StickDefinition.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "StoneCore/StoneDefinition.h"
#include "StonePhaseA_Mining/StoneActor.h"

namespace
{
	constexpr float PocketStoneMassKg = 2.f;
	constexpr float BuildingStoneMassKg = 40.f;
	/** The greybox stone mesh grown to a building stone. */
	constexpr float BuildingStoneScale = 2.4f;
}

AStoneActor* LooseResources::SpawnStone(UWorld* World, const FVector& Location, bool bBuildingStone)
{
	AStoneActor* Stone = World ? World->SpawnActor<AStoneActor>(AStoneActor::StaticClass(), Location, FRotator(0.f, FMath::FRand() * 360.f, 0.f)) : nullptr;
	if (!Stone)
	{
		return nullptr;
	}
	UStoneDefinition* Definition = NewObject<UStoneDefinition>(Stone);
	Definition->StoneName = bBuildingStone ? TEXT("BuildingStone") : TEXT("SmallStone");
	Definition->Type = EStoneType::Granite;
	Definition->Shape = bBuildingStone ? EStoneShape::Building : EStoneShape::Small;
	Definition->Role = EStoneRole::None;
	Definition->Dimensions = bBuildingStone ? FVector(40.f, 30.f, 26.f) : FVector(13.f, 10.f, 9.f);
	Definition->Mass = bBuildingStone ? BuildingStoneMassKg : PocketStoneMassKg;
	Definition->Volume = Definition->Dimensions.X * Definition->Dimensions.Y * Definition->Dimensions.Z * 1e-6f;
	Definition->CompressiveStrength = 180.f;
	Definition->TensileStrength = 15.f;
	Definition->Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Greybox/Meshes/SM_GB_Stone.SM_GB_Stone")));
	Stone->InitializeFromDefinition(Definition);
	if (Stone->MeshComponent && !Stone->MeshComponent->GetStaticMesh())
	{
		Stone->MeshComponent->SetStaticMesh(ForestItems::GetMesh(EForestItem::Stone));
	}
	if (Stone->MeshComponent && bBuildingStone)
	{
		Stone->MeshComponent->SetWorldScale3D(FVector(BuildingStoneScale));
	}
	return Stone;
}

AStickActor* LooseResources::SpawnStick(UWorld* World, const FVector& Location, const FRotator& Rotation)
{
	AStickActor* Stick = World ? World->SpawnActor<AStickActor>(AStickActor::StaticClass(), Location, Rotation) : nullptr;
	if (!Stick)
	{
		return nullptr;
	}
	UStickDefinition* Definition = NewObject<UStickDefinition>(Stick);
	Definition->StickName = TEXT("RawStick");
	Definition->Shape = EStickShape::Raw;
	Definition->Size = EStickSize::Short;
	Definition->Role = EStickRole::Crafting;
	Definition->Length = 55.f;
	Definition->Diameter = 2.8f;
	Definition->Mass = 0.2f;
	Definition->Volume = 0.0006f;
	Definition->Durability = 30.f;
	Definition->MaxStackSize = 20;
	Definition->Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Greybox/Meshes/SM_GB_Stick.SM_GB_Stick")));
	Stick->InitializeFromDefinition(Definition);
	if (Stick->MeshComponent && !Stick->MeshComponent->GetStaticMesh())
	{
		Stick->MeshComponent->SetStaticMesh(ForestItems::GetMesh(EForestItem::Stick));
	}
	return Stick;
}

bool LooseResources::IsPocketStone(const AStoneActor* Stone)
{
	const UStoneDefinition* Definition = Stone ? Stone->GetDefinition() : nullptr;
	return Definition && Definition->Mass <= PocketStoneMassKg + 0.5f;
}
