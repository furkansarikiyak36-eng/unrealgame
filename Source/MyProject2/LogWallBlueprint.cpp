// Copyright Epic Games, Inc. All Rights Reserved.

#include "LogWallBlueprint.h"
#include "LogConstructionActor.h"
#include "ForestTypes.h"
#include "ForestOutline.h"
#include "Components/LineBatchComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

ALogWallBlueprint::ALogWallBlueprint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Logs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Logs"));
	Logs->SetupAttachment(Root);
	Logs->SetMobility(EComponentMobility::Movable);
	Logs->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// The plan only answers view traces so the player can aim at it, nothing else collides with it.
	Ghosts = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Ghosts"));
	Ghosts->SetupAttachment(Root);
	Ghosts->SetMobility(EComponentMobility::Movable);
	Ghosts->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Ghosts->SetCollisionResponseToAllChannels(ECR_Ignore);
	Ghosts->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Ghosts->SetCastShadow(false);
	Ghosts->SetHiddenInGame(true);

	GhostLines = CreateDefaultSubobject<ULineBatchComponent>(TEXT("GhostLines"));
	GhostLines->SetupAttachment(Root);
	GhostLines->SetUsingAbsoluteLocation(true);
	GhostLines->SetUsingAbsoluteRotation(true);
	GhostLines->SetUsingAbsoluteScale(true);
	GhostLines->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GhostLines->SetCastShadow(false);
}

bool ALogWallBlueprint::ComputeSlots(const FVector& InStart, const FVector& InEnd, float InPhase, bool bFitExactly, TArray<FTransform>& OutSlots, FVector& OutEnd)
{
	OutSlots.Reset();
	const UStaticMesh* LogMesh = ForestItems::GetMesh(EForestItem::Log);
	if (!LogMesh)
	{
		return false;
	}

	FVector Dir = InEnd - InStart;
	Dir.Z = 0.f;
	const float Distance = Dir.Size();
	if (Distance < 50.f)
	{
		return false;
	}
	Dir /= Distance;

	const FBox Box = LogMesh->GetBoundingBox();
	const FVector LogScale = ForestItems::GetItemScale(EForestItem::Log);
	const FVector Size = Box.GetSize() * LogScale;
	const float Overhang = Size.Y * 0.6f;   // log ends stick out past the corner
	const float Pitch = Size.X * 0.96f;     // neighbouring logs overlap a little at the joints
	const float RowHeight = Size.Z * 0.74f; // rows sink into each other so the wall reads as one piece

	const float Span = Distance + 2.f * Overhang;
	const int32 Count = FMath::Clamp(FMath::RoundToInt(Span / Pitch), 1, 8);
	const float Stretch = bFitExactly ? Span / (Count * Pitch) : 1.f;
	const float StepAlong = Pitch * Stretch;

	OutEnd = InStart + Dir * (Count * StepAlong - 2.f * Overhang);
	OutEnd.Z = InStart.Z;

	const FQuat WallRotation = FRotator(0.f, Dir.Rotation().Yaw, 0.f).Quaternion();
	const FVector Scale(Stretch * LogScale.X, LogScale.Y, LogScale.Z);

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Along = -Overhang + StepAlong * (Index + 0.5f);
			const float Up = Size.Z * 0.5f - 4.f + RowHeight * (Row + InPhase * 0.5f);
			const FVector Center = InStart + Dir * Along + FVector(0.f, 0.f, Up);

			// Turn every other log over so neighbours do not look like copies.
			const bool bFlip = ((Row * 7 + Index * 3) % 2) == 1;
			const FQuat Rotation = WallRotation * FQuat(FVector::ForwardVector, bFlip ? PI : 0.f);
			const FVector Location = Center - Rotation.RotateVector(Box.GetCenter() * Scale);
			OutSlots.Add(FTransform(Rotation, Location, Scale));
		}
	}
	return true;
}

void ALogWallBlueprint::Init(const TArray<FTransform>& InSlots, const FVector& InStart, const FVector& InEnd, float InPhase)
{
	Slots = InSlots;
	Start = InStart;
	End = InEnd;
	Phase = InPhase;
	Filled = 0;

	UStaticMesh* LogMesh = ForestItems::GetMesh(EForestItem::Log);
	Logs->SetStaticMesh(LogMesh);
	Ghosts->SetStaticMesh(LogMesh);

	if (UMaterialInterface* Ghost = ForestItems::GetBuildGhostMaterial())
	{
		GhostMaterial = UMaterialInstanceDynamic::Create(Ghost, this);
		GhostMaterial->SetVectorParameterValue(TEXT("GhostColor"), FLinearColor(0.55f, 0.8f, 1.f));
		for (int32 Slot = 0; Slot < Ghosts->GetNumMaterials(); ++Slot)
		{
			Ghosts->SetMaterial(Slot, GhostMaterial);
		}
	}
	RefreshGhosts();
}

bool ALogWallBlueprint::AddLog()
{
	if (IsComplete())
	{
		return false;
	}
	Logs->AddInstance(Slots[Filled], /*bWorldSpace*/ true);
	++Filled;
	RefreshGhosts();
	return true;
}

void ALogWallBlueprint::RefreshGhosts()
{
	Ghosts->ClearInstances();
	GhostLines->Flush();
	const UStaticMesh* LogMesh = Ghosts->GetStaticMesh();
	for (int32 Index = Filled; Index < Slots.Num(); ++Index)
	{
		Ghosts->AddInstance(Slots[Index], /*bWorldSpace*/ true);
		const FLinearColor Color = Index == Filled ? FLinearColor::White : FLinearColor(0.45f, 0.65f, 0.85f);
		ForestOutline::DrawPiece(GhostLines, EForestItem::Log, LogMesh, Slots[Index], Color);
	}
}

void ALogWallBlueprint::ApplyChinking()
{
	bIsChinked = true;
	UpdateVisualColor();
}

void ALogWallBlueprint::UpdateVisualColor()
{
	if (!Logs)
	{
		return;
	}

	for (int32 MatIdx = 0; MatIdx < Logs->GetNumMaterials(); ++MatIdx)
	{
		UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(Logs->GetMaterial(MatIdx));
		if (!DynMat)
		{
			DynMat = Logs->CreateAndSetMaterialInstanceDynamic(MatIdx);
		}
		if (DynMat)
		{
			// Değer arttıkça (chinking uygulandığında): Koyu kütük renginden açık harç dolgulu yalıtım tonuna geçer
			const FLinearColor ChinkedColor = bIsChinked
				? FLinearColor(0.85f, 0.78f, 0.65f)
				: FLinearColor(1.0f, 1.0f, 1.0f);

			DynMat->SetVectorParameterValue(TEXT("BaseColorTint"), ChinkedColor);
			DynMat->SetVectorParameterValue(TEXT("Color"), ChinkedColor);
			DynMat->SetVectorParameterValue(TEXT("Tint"), ChinkedColor);
		}
	}
}

void ALogWallBlueprint::CollapseWall(const FVector& ImpactDirection)
{
	if (Filled <= 0)
	{
		Destroy();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector BaseImpulse = (!ImpactDirection.IsNearlyZero())
		? ImpactDirection.GetSafeNormal() * 500.0f
		: FVector(FMath::RandRange(-200.f, 200.f), FMath::RandRange(-200.f, 200.f), 120.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Index = 0; Index < Filled; ++Index)
	{
		const FTransform& SlotTransform = Slots[Index];
		ALogConstructionActor* PhysicalLog = World->SpawnActor<ALogConstructionActor>(
			ALogConstructionActor::StaticClass(),
			SlotTransform,
			SpawnParams
		);

		if (PhysicalLog)
		{
			PhysicalLog->InitializeLog(EForestItem::Log, ELogMoistureState::Green);
			if (bIsChinked)
			{
				PhysicalLog->ApplyChinking();
			}

			const FVector LogImpulse = BaseImpulse + FVector(
				FMath::RandRange(-150.f, 150.f),
				FMath::RandRange(-150.f, 150.f),
				FMath::RandRange(60.f, 280.f)
			);
			PhysicalLog->TriggerChaosCollapse(LogImpulse);
		}
	}

	Logs->ClearInstances();
	Ghosts->ClearInstances();
	GhostLines->Flush();
	Filled = 0;
	Destroy();
}

