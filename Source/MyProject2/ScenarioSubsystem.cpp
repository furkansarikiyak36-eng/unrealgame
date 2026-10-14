// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioSubsystem.h"
#include "CarvableLog.h"
#include "FarmKit.h"
#include "LooseResources.h"
#include "WorldFeatures.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	FVector GroundAt(UWorld* World, const FVector& Where)
	{
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Where + FVector(0.f, 0.f, 2000.f), Where - FVector(0.f, 0.f, 2000.f), FCollisionObjectQueryParams(ECC_WorldStatic)))
		{
			return Hit.ImpactPoint;
		}
		return FVector(Where.X, Where.Y, 0.f);
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

	// More stones lying about the rocky west.
	for (int32 Index = 0; Index < 5; ++Index)
	{
		LooseResources::SpawnStone(World, GroundAt(World, FVector(-2000.f - 150.f * Index, -900.f + 200.f * Index, 0.f)) + FVector(0.f, 0.f, 30.f), Index == 0);
	}
	UE_LOG(LogTemp, Display, TEXT("[Senaryo] Dünya kuruldu: nehir, yar, yanık kulübe, %d not, yabani tahıl, hayvanlar, taşlar"), Notes.Num());
}
