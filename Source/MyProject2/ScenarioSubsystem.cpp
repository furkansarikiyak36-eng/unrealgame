// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioSubsystem.h"
#include "CarvableLog.h"
#include "WorldCore/ExcavationTerrainChunkSubsystem.h"
#include "FarmKit.h"
#include "LooseResources.h"
#include "RawMaterialNode.h"
#include "WorldFeatures.h"
#include "HarvestableActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	FVector GroundAt(UWorld* World, const FVector& Where)
	{
		// Diggable voxel ground: build it under this spot first, so things are placed on it and not under it.
		if (UExcavationTerrainChunkSubsystem* Ground = World->GetSubsystem<UExcavationTerrainChunkSubsystem>())
		{
			Ground->EnsureGroundAt(FVector(Where.X, Where.Y, 0.f));
		}
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Where + FVector(0.f, 0.f, 2000.f), Where - FVector(0.f, 0.f, 2000.f), FCollisionObjectQueryParams(ECC_WorldStatic)))
		{
			return Hit.ImpactPoint;
		}
		return FVector(Where.X, Where.Y, 0.f);
	}

	/** A tree or rock the player can cut, made the way the game mode makes the ones placed in the level. */
	void SpawnHarvestable(UWorld* World, const TCHAR* MeshName, const FVector& Ground, float Yaw, EHarvestType Type)
	{
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Greybox/Meshes/%s.%s"), MeshName, MeshName));
		AStaticMeshActor* Source = Mesh ? World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FRotator(0.f, Yaw, 0.f), Ground)) : nullptr;
		if (!Source)
		{
			return;
		}
		Source->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		Source->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		if (AHarvestableActor* Harvestable = World->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), Source->GetActorTransform()))
		{
			Harvestable->InitFrom(Source->GetStaticMeshComponent(), Type);
		}
		Source->Destroy();
	}
}

UScenarioSubsystem* UScenarioSubsystem::Get(const UWorld* World)
{
	return World ? World->GetSubsystem<UScenarioSubsystem>() : nullptr;
}

bool UScenarioSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

const TArray<TPair<FString, FString>>& UScenarioSubsystem::StoryNotes()
{
	static const TArray<TPair<FString, FString>> Notes = {
		{ TEXT("Ayaz kampı, 1. gün"), TEXT("Kar erken geldi. Ateşi taşla çevir, yoksa gece seni alır. Kayalardan taş, ağaçtan dal topladık; ilk gece ateşin başında uyumadık.") },
		{ TEXT("Nehir"), TEXT("Öte yakada koyunlar, tavuklar, yabani tahıl var. Suya girersen ıslanırsın, ıslanan donar. İki direk ve bir kirişle köprü kurmalıydık; kurmadık.") },
		{ TEXT("Yar"), TEXT("Yarın tepesinde iyi taş var. İp olmadan çıkılmıyor; deriden, yünden ip büktük ve tepeye astık.") },
		{ TEXT("Üçüncü gece"), TEXT("Her üçüncü gece geliyorlar, her seferinde daha çok. Sivri kazıklar onları yavaşlattı ama durdurmadı. Duvarı kalın ör.") },
		{ TEXT("Yanık kulübe"), TEXT("Kulübe yandı; ateş duvara fazla yakındı. Çatıyı bitiremedik. Kim bunu okuyorsa: ateşi taşla çevir, sakın yalnız kalma.") },
	};
	return Notes;
}

bool UScenarioSubsystem::RecordNote(const FString& Title)
{
	if (Title.IsEmpty() || Journal.Contains(Title))
	{
		return false;
	}
	Journal.Add(Title);
	return true;
}

void UScenarioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// The mechanics test has its own areas all over the map: no scenery in its way.
	if (FParse::Param(FCommandLine::Get(), TEXT("MechanicsDemoQuit")) || FParse::Param(FCommandLine::Get(), TEXT("MechanicsDemo"))
		|| FParse::Param(FCommandLine::Get(), TEXT("NoScenario")))
	{
		return;
	}
	// Small demo stages lay themselves out: the scenario's river and cliff would not fit on them.
	if (InWorld.GetMapName().Contains(TEXT("Demo_RawMaterials")) || InWorld.GetMapName().Contains(TEXT("LogShowcase")))
	{
		return;
	}
	LayOut();
}

void UScenarioSubsystem::LayOut()
{
	UWorld* World = GetWorld();
	if (bLaidOut || !World)
	{
		return;
	}
	bLaidOut = true;
	const TArray<TPair<FString, FString>>& Notes = StoryNotes();
	auto Note = [&](int32 Index, const FVector& Where)
	{
		ARemnantNote::Place(World, GroundAt(World, Where), Notes[Index].Key, Notes[Index].Value);
	};

	// Orman: the first note near the start, the burnt cabin of those before.
	Note(0, FVector(300.f, 300.f, 0.f));
	const FVector Cabin = GroundAt(World, FVector(-800.f, -2500.f, 0.f));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FRotator Yaw(0.f, Index % 2 == 0 ? 0.f : 90.f, 0.f);
		const FVector Where = Cabin + FVector(Index == 1 ? 180.f : (Index == 3 ? -180.f : 0.f), Index == 0 ? -180.f : (Index == 2 ? 180.f : 0.f), 36.f + 60.f * (Index / 2));
		if (ACarvableLog* Log = ACarvableLog::SpawnLog(World, FTransform(Yaw, Where)))
		{
			Log->PlaceAt(Log->GetLayTransform(Where - FVector(0.f, 0.f, 36.f), Yaw.Yaw), true);
			Log->SetDamage(0.95f);
		}
	}
	Note(4, Cabin + FVector(0.f, 0.f, 0.f));

	// Nehir kıyısı: a river across the map, the farm on the far bank.
	ARiverStrip::Spawn(World, GroundAt(World, FVector(2600.f, 0.f, 0.f)), FVector2D(250.f, 3500.f));
	Note(1, FVector(2150.f, 400.f, 0.f));
	FarmKit::MakeWildPatch(World, GroundAt(World, FVector(3200.f, 600.f, 0.f)));
	FarmKit::MakeWildPatch(World, GroundAt(World, FVector(3200.f, -400.f, 0.f)));
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FarmKit::SpawnAnimal(World, GroundAt(World, FVector(3150.f, 1300.f + 300.f * Index, 0.f)) + FVector(0.f, 0.f, 40.f), EAnimalSpecies::Sheep);
		FarmKit::SpawnAnimal(World, GroundAt(World, FVector(3300.f, -1200.f - 200.f * Index, 0.f)) + FVector(0.f, 0.f, 20.f), EAnimalSpecies::Chicken);
	}

	// Kayalık: a cliff that needs a rope, building stones and small stones on top.
	const FVector CliffBase = GroundAt(World, FVector(-3000.f, 1500.f, 0.f));
	if (ACliffBlock* Cliff = ACliffBlock::Spawn(World, CliffBase, FVector(900.f, 2200.f, 450.f)))
	{
		const float Top = Cliff->GetTopZ();
		for (int32 Index = 0; Index < 6; ++Index)
		{
			LooseResources::SpawnStone(World, FVector(-3150.f + 120.f * (Index % 3), 1100.f + 300.f * Index, Top + 40.f), Index < 3);
		}
		ARemnantNote::Place(World, FVector(-3100.f, 2300.f, Top), Notes[3].Key, Notes[3].Value);
	}
	Note(2, FVector(-2400.f, 1500.f, 0.f));

	// Orman: more trees; Kayalık: rocks to break for stone.
	const FVector2D Trees[] = { { -600.f, 900.f }, { -300.f, 1500.f }, { 200.f, 1900.f }, { -900.f, -400.f }, { -1100.f, 400.f },
		{ 400.f, -1000.f }, { 1100.f, 1500.f }, { 1300.f, -700.f }, { -200.f, -1900.f }, { 600.f, 2400.f } };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Trees); ++Index)
	{
		SpawnHarvestable(World, Index % 3 == 2 ? TEXT("SM_GB_Tree_Broad") : TEXT("SM_GB_Tree_Pine"), GroundAt(World, FVector(Trees[Index], 0.f)), 37.f * Index, EHarvestType::Tree);
	}
	const FVector2D Rocks[] = { { -1700.f, -300.f }, { -1900.f, 500.f }, { -2200.f, -1500.f }, { -1600.f, -2200.f }, { -2400.f, 300.f }, { -1800.f, 2600.f } };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Rocks); ++Index)
	{
		SpawnHarvestable(World, TEXT("SM_GB_Rock"), GroundAt(World, FVector(Rocks[Index], 0.f)), 53.f * Index, EHarvestType::Rock);
	}

	// More stones lying about the rocky west.
	for (int32 Index = 0; Index < 5; ++Index)
	{
		LooseResources::SpawnStone(World, GroundAt(World, FVector(-2000.f - 150.f * Index, -900.f + 200.f * Index, 0.f)) + FVector(0.f, 0.f, 30.f), Index == 0);
	}
	// Raw materials that bind and seal the pieces: fibre, vines, bark and bones in the forest, clay on the near river bank,
	// flint among the western rocks.
	struct FRawSpot { float X, Y; ERawSourceKind Kind; };
	const FRawSpot RawSpots[] = {
		{ 600.f, 600.f, ERawSourceKind::FiberBush }, { -500.f, 1200.f, ERawSourceKind::FiberBush }, { 900.f, -300.f, ERawSourceKind::FiberBush },
		{ -400.f, -1000.f, ERawSourceKind::FiberBush }, { 1600.f, 900.f, ERawSourceKind::FiberBush },
		{ 0.f, 2200.f, ERawSourceKind::Vines }, { 1500.f, -1300.f, ERawSourceKind::Vines }, { -1300.f, 1000.f, ERawSourceKind::Vines },
		{ -700.f, 200.f, ERawSourceKind::DeadTree }, { 800.f, 1800.f, ERawSourceKind::DeadTree },
		{ -500.f, -2300.f, ERawSourceKind::BonePile },
		{ 2250.f, -800.f, ERawSourceKind::ClayBank }, { 2250.f, 1000.f, ERawSourceKind::ClayBank }, { 2250.f, 2000.f, ERawSourceKind::ClayBank },
		{ -2100.f, -700.f, ERawSourceKind::FlintOutcrop }, { -2300.f, 900.f, ERawSourceKind::FlintOutcrop }, { -1900.f, -1900.f, ERawSourceKind::FlintOutcrop } };
	for (const FRawSpot& Spot : RawSpots)
	{
		ARawMaterialNode::Spawn(World, GroundAt(World, FVector(Spot.X, Spot.Y, 0.f)), Spot.Kind);
	}

	UE_LOG(LogTemp, Display, TEXT("[Senaryo] Dünya kuruldu: nehir, yar, yanık kulübe, %d not, yabani tahıl, hayvanlar, taşlar, %d ham madde kaynağı"), Notes.Num(), int32(UE_ARRAY_COUNT(RawSpots)));
}
