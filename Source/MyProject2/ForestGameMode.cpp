// Copyright Epic Games, Inc. All Rights Reserved.

#include "ForestGameMode.h"
#include "LogShowcase.h"
#include "WorldCore/ExcavationTerrainChunkSubsystem.h"
#include "ForestCharacter.h"
#include "ForestHUD.h"
#include "HarvestableActor.h"
#include "StoneMasonryActor.h"
#include "LogConstructionActor.h"
#include "OpeningBuckActor.h"
#include "LogWallBlueprint.h"
#include "PickupItem.h"
#include "CarvableLog.h"
#include "MechanicsDemo.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

AForestGameMode::AForestGameMode()
{
	DefaultPawnClass = AForestCharacter::StaticClass();
	HUDClass = AForestHUD::StaticClass();
	bSpawnDemoShowcase = true;
}

void AForestGameMode::StartPlay()
{
	ConvertHarvestables();

	// The wood chain demo level keeps its stage clear of the building stations.
	const bool bDemoLevel = TActorIterator<AMechanicsDemo>(GetWorld()) || TActorIterator<ALogShowcase>(GetWorld());
	if (bSpawnDemoShowcase && !bDemoLevel)
	{
		SetupDemoShowcase();
	}

	Super::StartPlay();

	// -MechanicsDemo plays the wood chain test by itself on start; -MechanicsDemoQuit also exits when it is done (headless runs).
	if (FParse::Param(FCommandLine::Get(), TEXT("MechanicsDemo")) || FParse::Param(FCommandLine::Get(), TEXT("MechanicsDemoQuit")))
	{
		AMechanicsDemo::Start(GetWorld(), /*bAuto*/ true, FParse::Param(FCommandLine::Get(), TEXT("MechanicsDemoQuit")));
	}
}

void AForestGameMode::ConvertHarvestables()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AStaticMeshActor*> Candidates;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		Candidates.Add(*It);
	}

	for (AStaticMeshActor* Source : Candidates)
	{
		UStaticMeshComponent* SourceMesh = Source->GetStaticMeshComponent();
		const UStaticMesh* StaticMesh = SourceMesh ? SourceMesh->GetStaticMesh() : nullptr;
		if (!StaticMesh)
		{
			continue;
		}

		const FString Name = StaticMesh->GetName();
		EHarvestType Type;
		// Greybox trees and rocks (SM_GB_Tree_*, SM_GB_Rock*) placed straight from the content browser.
		if (Name.StartsWith(TEXT("SM_GB_Tree")))
		{
			Type = EHarvestType::Tree;
		}
		else if (Name.StartsWith(TEXT("SM_GB_Rock")) && Source->GetActorScale3D().GetMax() <= MaxRockScale)
		{
			Type = EHarvestType::Rock;
		}
		else
		{
			continue;
		}

		AHarvestableActor* Harvestable = World->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), Source->GetActorTransform());
		if (Harvestable)
		{
			Harvestable->InitFrom(SourceMesh, Type);
			Source->SetActorHiddenInGame(true);
			Source->SetActorEnableCollision(false);
		}
	}
}

void AForestGameMode::SetupDemoShowcase()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector Origin = FVector(0.f, 0.f, 200.f);
	FVector Forward = FVector(1.f, 0.f, 0.f);
	FVector Right = FVector(0.f, 1.f, 0.f);

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		Origin = It->GetActorLocation();
		Forward = It->GetActorForwardVector().GetSafeNormal2D();
		Right = It->GetActorRightVector().GetSafeNormal2D();
		break;
	}

	auto SnapToGround = [World](const FVector& Pos) -> FVector
	{
		if (UExcavationTerrainChunkSubsystem* Ground = World->GetSubsystem<UExcavationTerrainChunkSubsystem>())
		{
			Ground->EnsureGroundAt(FVector(Pos.X, Pos.Y, 0.f));
		}
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(DemoSnap), false);
		if (World->LineTraceSingleByChannel(Hit, Pos + FVector(0, 0, 600.f), Pos - FVector(0, 0, 2000.f), ECC_Visibility, Params))
		{
			return Hit.ImpactPoint;
		}
		return Pos;
	};

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// =========================================================================
	// İSTASYON 1: Kuru Taş Duvar & Harçlı Taş Temel (Stone Masonry Station)
	// =========================================================================
	const FVector StoneBase = SnapToGround(Origin + Forward * 550.f - Right * 450.f);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector Pos = StoneBase + Right * (Index * 130.f);
		if (AStoneMasonryActor* Stone = World->SpawnActor<AStoneMasonryActor>(AStoneMasonryActor::StaticClass(), FTransform(Pos), SpawnParams))
		{
			if (Index < 2)
			{
				Stone->InitializeStone(EMasonryType::DryStone, true, true);
			}
			else
			{
				Stone->InitializeStone(EMasonryType::DryStone, true, true);
				Stone->ApplyMortar(); // Convert to mortared stone
			}
		}
	}

	// =========================================================================
	// İSTASYON 2: Hibrit Kütük Duvar, Chinking & Alçı Sıva (Hybrid Log Wall)
	// =========================================================================
	const FVector WallStart = SnapToGround(Origin + Forward * 650.f - Right * 150.f);
	const FVector WallEnd = SnapToGround(Origin + Forward * 650.f + Right * 250.f);
	TArray<FTransform> WallSlots;
	FVector RealWallEnd;
	if (ALogWallBlueprint::ComputeSlots(WallStart, WallEnd, 0.f, false, WallSlots, RealWallEnd))
	{
		if (ALogWallBlueprint* FinishedWall = World->SpawnActor<ALogWallBlueprint>(ALogWallBlueprint::StaticClass(), FTransform(WallStart), SpawnParams))
		{
			FinishedWall->Init(WallSlots, WallStart, RealWallEnd, 0.f);
			while (FinishedWall->AddLog())
			{
			}
			FinishedWall->ApplyChinking(); // Chink the whole wall with plaster sealant
		}
	}

	// Bitişiğinde inşa edilmeyi bekleyen hayalet duvar (Ghost Blueprint Wall)
	const FVector GhostStart = RealWallEnd;
	const FVector GhostEnd = SnapToGround(GhostStart + Forward * 400.f);
	TArray<FTransform> GhostSlots;
	FVector RealGhostEnd;
	if (ALogWallBlueprint::ComputeSlots(GhostStart, GhostEnd, 1.f, false, GhostSlots, RealGhostEnd))
	{
		if (ALogWallBlueprint* GhostWall = World->SpawnActor<ALogWallBlueprint>(ALogWallBlueprint::StaticClass(), FTransform(GhostStart), SpawnParams))
		{
			GhostWall->Init(GhostSlots, GhostStart, RealGhostEnd, 1.f);
			GhostWall->AddLog(); // 1 kütük eklenmiş, kalanı oyuncuyu bekliyor
		}
	}

	// =========================================================================
	// İSTASYON 3: Pencere & Kapı Açıklık Kasaları (%6 Settling Allowance)
	// =========================================================================
	const FVector WindowPos = SnapToGround(Origin + Forward * 850.f - Right * 250.f);
	if (AOpeningBuckActor* WindowBuck = World->SpawnActor<AOpeningBuckActor>(AOpeningBuckActor::StaticClass(), FTransform(WindowPos), SpawnParams))
	{
		WindowBuck->InitializeBuck(EOpeningType::Window, 90.f, 120.f, true);
	}

	const FVector DoorPos = SnapToGround(Origin + Forward * 850.f - Right * 100.f);
	if (AOpeningBuckActor* DoorBuck = World->SpawnActor<AOpeningBuckActor>(AOpeningBuckActor::StaticClass(), FTransform(DoorPos), SpawnParams))
	{
		DoorBuck->InitializeBuck(EOpeningType::Door, 90.f, 210.f, true);
	}

	// =========================================================================
	// İSTASYON 4: Zincirleme Yapısal Çöküş Test Standı (Cascade Failure Demo)
	// =========================================================================
	const FVector CrashBase = SnapToGround(Origin + Forward * 750.f + Right * 480.f);
	if (AStoneMasonryActor* PillarStone = World->SpawnActor<AStoneMasonryActor>(AStoneMasonryActor::StaticClass(), FTransform(CrashBase), SpawnParams))
	{
		PillarStone->InitializeStone(EMasonryType::DryStone);

		// Taşın üstüne oturan alt kütük (greybox: taş 30 cm, kütük çapı 70 cm, kiriş 25 cm)
		const FVector LogPos1 = CrashBase + FVector(0.f, 0.f, 30.f + 35.f);
		if (ALogConstructionActor* Log1 = World->SpawnActor<ALogConstructionActor>(ALogConstructionActor::StaticClass(), FTransform(LogPos1), SpawnParams))
		{
			Log1->InitializeLog(EForestItem::Log, ELogMoistureState::Green);
			PillarStone->AddSupportingNode(Log1);

			// Onun üstüne oturan üst kütük
			const FVector LogPos2 = LogPos1 + FVector(0.f, 0.f, 70.f);
			if (ALogConstructionActor* Log2 = World->SpawnActor<ALogConstructionActor>(ALogConstructionActor::StaticClass(), FTransform(LogPos2), SpawnParams))
			{
				Log2->InitializeLog(EForestItem::Log, ELogMoistureState::Green);
				Log1->AddSupportingNode(Log2);

				// En tepedeki kiriş
				const FVector BeamPos = LogPos2 + FVector(0.f, 0.f, 35.f + 12.5f);
				if (ALogConstructionActor* TopBeam = World->SpawnActor<ALogConstructionActor>(ALogConstructionActor::StaticClass(), FTransform(BeamPos), SpawnParams))
				{
					TopBeam->InitializeLog(EForestItem::Beam, ELogMoistureState::AirDry);
					Log2->AddSupportingNode(TopBeam);
				}
			}
		}
	}

	// =========================================================================
	// İSTASYON 5: Malzeme & Alet Deposu (Materials & Tool Depot)
	// =========================================================================
	const FVector MaterialBase = SnapToGround(Origin + Forward * 350.f);
	for (int32 Index = 0; Index < 6; ++Index)
	{
		// Side by side along Right, each log pointing away from the player (70 cm thick).
		const FVector LogPos = MaterialBase + Right * (Index * 80.f - 200.f) + FVector(0.f, 0.f, 40.f);
		ACarvableLog::SpawnLog(World, FTransform(Forward.Rotation(), LogPos));
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector StonePos = MaterialBase - Forward * 260.f + Right * (Index * 40.f - 60.f) + FVector(0.f, 0.f, 20.f);
		APickupItem::Spawn(World, EForestItem::Stone, 2, FTransform(StonePos), FVector::ZeroVector);
	}
}
