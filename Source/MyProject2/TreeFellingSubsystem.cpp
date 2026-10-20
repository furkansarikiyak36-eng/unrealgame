// Copyright Epic Games, Inc. All Rights Reserved.

#include "TreeFellingSubsystem.h"
#include "HarvestableActor.h"
#include "ForestCharacter.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

bool UTreeFellingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UTreeFellingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	ConvertFellableTrees();
}

int32 UTreeFellingSubsystem::ConvertFellableTrees()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	TArray<ASkeletalMeshActor*> Sources;
	for (TActorIterator<ASkeletalMeshActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(FellableTag) && !It->IsHidden())
		{
			Sources.Add(*It);
		}
	}

	int32 Count = 0;
	for (ASkeletalMeshActor* Source : Sources)
	{
		const USkeletalMeshComponent* Skin = Source->GetSkeletalMeshComponent();
		if (!Skin || !Skin->GetSkeletalMeshAsset())
		{
			continue;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AHarvestableActor* Tree = World->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), Source->GetActorTransform(), Params))
		{
			Tree->InitFromSkeletal(Skin);
			Source->SetActorHiddenInGame(true);
			Source->SetActorEnableCollision(false);
			++Count;
		}
	}
	// Level-placed static trees (e.g. KiteDemo) with the same tag.
	TArray<AStaticMeshActor*> StaticSources;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(FellableTag) && !It->IsHidden() && It->GetStaticMeshComponent() && It->GetStaticMeshComponent()->GetStaticMesh())
		{
			StaticSources.Add(*It);
		}
	}
	for (AStaticMeshActor* Source : StaticSources)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AHarvestableActor* Tree = World->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), Source->GetActorTransform(), Params))
		{
			Tree->InitFromStaticTree(Source->GetStaticMeshComponent());
			Source->SetActorHiddenInGame(true);
			Source->SetActorEnableCollision(false);
			++Count;
		}
	}

	// Level-placed rocks tagged "Breakable".
	TArray<AStaticMeshActor*> RockSources;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(BreakableTag) && !It->IsHidden() && It->GetStaticMeshComponent() && It->GetStaticMeshComponent()->GetStaticMesh())
		{
			RockSources.Add(*It);
		}
	}
	for (AStaticMeshActor* Source : RockSources)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AHarvestableActor* Rock = World->SpawnActor<AHarvestableActor>(AHarvestableActor::StaticClass(), Source->GetActorTransform(), Params))
		{
			Rock->InitFromStaticRock(Source->GetStaticMeshComponent());
			Source->SetActorHiddenInGame(true);
			Source->SetActorEnableCollision(false);
			++Count;
		}
	}

	Converted += Count;
	UE_LOG(LogTemp, Log, TEXT("TreeFelling: %d converted (%d static trees, %d rocks, rest skeletal trees)"), Count, StaticSources.Num(), RockSources.Num());
	return Count;
}

namespace
{
/**
 * Waist-high sweep straight ahead, then one down to the ground ahead, then a steep one to the ground just in front
 * (flat stones); the first harvestable that Accept takes.
 */
template <typename FAccept>
AHarvestableActor* FindInFront(UWorld* World, AActor* Chopper, float Reach, FHitResult& OutHit, FAccept Accept)
{
	const FVector Forward = Chopper->GetActorForwardVector().GetSafeNormal2D();
	const FVector Start = Chopper->GetActorLocation() + FVector(0.f, 0.f, 20.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TreeChop), false, Chopper);
	const FVector Ends[] = { Start + Forward * Reach, Start + Forward * Reach * 0.8f - FVector(0.f, 0.f, 140.f),
		Start + Forward * Reach * 0.55f - FVector(0.f, 0.f, 200.f) };
	for (const FVector& End : Ends)
	{
		TArray<FHitResult> Hits;
		World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(25.f), Params);
		for (const FHitResult& Hit : Hits)
		{
			AHarvestableActor* Harvestable = Cast<AHarvestableActor>(Hit.GetActor());
			if (Harvestable && Accept(Harvestable))
			{
				OutHit = Hit;
				return Harvestable;
			}
		}
	}
	// Last: small pieces lying on the ground just ahead, which the sweeps above can miss (the ground stops them first).
	TArray<FOverlapResult> Overlaps;
	const FVector Probe = Chopper->GetActorLocation() + Forward * Reach * 0.5f - FVector(0.f, 0.f, 60.f);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	World->OverlapMultiByObjectType(Overlaps, Probe, FQuat::Identity, Objects, FCollisionShape::MakeSphere(70.f), Params);
	AHarvestableActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AHarvestableActor* Harvestable = Cast<AHarvestableActor>(Overlap.GetActor());
		if (Harvestable && Accept(Harvestable))
		{
			const float DistSq = FVector::DistSquared(Harvestable->GetActorLocation(), Probe);
			if (DistSq < BestDistSq)
			{
				Best = Harvestable;
				BestDistSq = DistSq;
			}
		}
	}
	if (Best)
	{
		OutHit = FHitResult(Best, Cast<UPrimitiveComponent>(Best->GetRootComponent()), Best->GetActorLocation(), -Forward);
	}
	return Best;
}
}

bool UTreeFellingSubsystem::ChopFrom(AActor* Chopper)
{
	UWorld* World = GetWorld();
	if (!World || !Chopper)
	{
		return false;
	}
	// The tool follows the target: axe on trees and logs, pickaxe on rocks.
	FHitResult Hit;
	AHarvestableActor* Harvestable = FindInFront(World, Chopper, ChopReachCm, Hit, [](AHarvestableActor*) { return true; });
	if (!Harvestable)
	{
		return false;
	}
	const bool bDone = Harvestable->ApplyHit(ChopDamage, Hit, Chopper->GetActorForwardVector().GetSafeNormal2D());
	UE_LOG(LogTemp, Log, TEXT("TreeFelling: hit %s, %.0f%% left%s"), *Harvestable->GetDisplayName(),
		Harvestable->GetHealthPercent() * 100.f, bDone ? TEXT(" -> falls/splits/breaks") : TEXT(""));
	return true;
}

bool UTreeFellingSubsystem::CarveFrom(AActor* Chopper)
{
	UWorld* World = GetWorld();
	if (!World || !Chopper)
	{
		return false;
	}
	FHitResult Hit;
	AHarvestableActor* Stone = FindInFront(World, Chopper, ChopReachCm, Hit, [](AHarvestableActor* H) { return H->IsCarvable(); });
	if (!Stone)
	{
		return false;
	}
	const bool bCarved = Stone->CarveHit();
	UE_LOG(LogTemp, Log, TEXT("TreeFelling: chisel on %s%s"), *Stone->GetDisplayName(), bCarved ? TEXT(" -> carved") : TEXT(""));
	return true;
}

bool UTreeFellingSubsystem::IsTickable() const
{
	return Converted > 0 && !IsTemplate();
}

void UTreeFellingSubsystem::Tick(float DeltaSeconds)
{
	Cooldown = FMath::Max(0.f, Cooldown - DeltaSeconds);
	APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	// The project's character swings its own axe.
	if (!Pawn || Pawn->IsA<AForestCharacter>() || Cooldown > 0.f)
	{
		return;
	}
	if (Controller->WasInputKeyJustPressed(EKeys::LeftMouseButton) || Controller->WasInputKeyJustPressed(EKeys::Gamepad_RightTrigger))
	{
		Cooldown = ChopCooldownSeconds;
		ChopFrom(Pawn);
	}
	else if (Controller->WasInputKeyJustPressed(EKeys::RightMouseButton) || Controller->WasInputKeyJustPressed(EKeys::Gamepad_LeftTrigger))
	{
		Cooldown = ChopCooldownSeconds;
		CarveFrom(Pawn);
	}
}
