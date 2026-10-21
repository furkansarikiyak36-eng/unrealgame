// Copyright Epic Games, Inc. All Rights Reserved.

#include "RawMaterialNode.h"
#include "ForestCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

namespace
{
	UStaticMesh* BasicShape(const TCHAR* Name)
	{
		return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name));
	}

	/** A greybox look per kind: an engine shape (100 cm, pivot in the middle), its size and colour. */
	struct FSourceLook
	{
		const TCHAR* Shape;
		FVector Scale;
		FLinearColor Color;
	};

	FSourceLook LookOf(ERawSourceKind Kind)
	{
		switch (Kind)
		{
		case ERawSourceKind::Vines:        return { TEXT("Cone"),     FVector(0.6f, 0.6f, 1.6f),  FLinearColor(0.12f, 0.32f, 0.08f) };
		case ERawSourceKind::DeadTree:     return { TEXT("Cylinder"), FVector(0.4f, 0.4f, 2.6f),  FLinearColor(0.32f, 0.27f, 0.22f) };
		case ERawSourceKind::ClayBank:     return { TEXT("Cube"),     FVector(2.0f, 1.4f, 0.25f), FLinearColor(0.58f, 0.34f, 0.2f) };
		case ERawSourceKind::FlintOutcrop: return { TEXT("Cone"),     FVector(0.9f, 0.9f, 0.7f),  FLinearColor(0.2f, 0.22f, 0.28f) };
		case ERawSourceKind::BonePile:     return { TEXT("Sphere"),   FVector(0.9f, 0.6f, 0.25f), FLinearColor(0.88f, 0.85f, 0.76f) };
		case ERawSourceKind::FiberBush:
		default:                           return { TEXT("Sphere"),   FVector(1.2f, 1.2f, 0.8f),  FLinearColor(0.3f, 0.52f, 0.16f) };
		}
	}

	FRawMaterialDrop Drop(EForestItem Item, int32 Min, int32 Max, float Probability = 1.f)
	{
		FRawMaterialDrop Result;
		Result.Item = Item;
		Result.Min = Min;
		Result.Max = Max;
		Result.Probability = Probability;
		return Result;
	}

	bool SameDrops(const TArray<FRawMaterialDrop>& A, const TArray<FRawMaterialDrop>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index].Item != B[Index].Item || A[Index].Min != B[Index].Min || A[Index].Max != B[Index].Max
				|| !FMath::IsNearlyEqual(A[Index].Probability, B[Index].Probability))
			{
				return false;
			}
		}
		return true;
	}

	/** Drops left exactly as another kind's defaults: they came with the kind, not set by hand. */
	bool IsOtherKindsDefault(const TArray<FRawMaterialDrop>& Drops, ERawSourceKind Kind)
	{
		const UEnum* Kinds = StaticEnum<ERawSourceKind>();
		for (int32 Index = 0; Kinds && Index < Kinds->NumEnums() - 1; ++Index)
		{
			const ERawSourceKind Other = static_cast<ERawSourceKind>(Kinds->GetValueByIndex(Index));
			if (Other != Kind && SameDrops(Drops, ARawMaterialNode::DefaultDrops(Other)))
			{
				return true;
			}
		}
		return false;
	}

	/** An empty source shrinks to this much of its size. */
	constexpr float DepletedScale = 0.45f;
}

ARawMaterialNode::ARawMaterialNode()
{
	PrimaryActorTick.bCanEverTick = false;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

TArray<FRawMaterialDrop> ARawMaterialNode::DefaultDrops(ERawSourceKind Kind)
{
	switch (Kind)
	{
	case ERawSourceKind::Vines:        return { Drop(EForestItem::Vine, 1, 3) };
	case ERawSourceKind::DeadTree:     return { Drop(EForestItem::Bark, 1, 3), Drop(EForestItem::Resin, 1, 1, 0.35f) };
	case ERawSourceKind::ClayBank:     return { Drop(EForestItem::Clay, 1, 2) };
	case ERawSourceKind::FlintOutcrop: return { Drop(EForestItem::Flint, 1, 2) };
	case ERawSourceKind::BonePile:     return { Drop(EForestItem::Bone, 1, 2) };
	case ERawSourceKind::FiberBush:
	default:                           return { Drop(EForestItem::PlantFiber, 2, 4), Drop(EForestItem::Vine, 1, 2, 0.4f) };
	}
}

FString ARawMaterialNode::KindName(ERawSourceKind Kind)
{
	switch (Kind)
	{
	case ERawSourceKind::Vines:        return TEXT("Asma");
	case ERawSourceKind::DeadTree:     return TEXT("Kuru Ağaç");
	case ERawSourceKind::ClayBank:     return TEXT("Kil Yatağı");
	case ERawSourceKind::FlintOutcrop: return TEXT("Çakmak Taşı Damarı");
	case ERawSourceKind::BonePile:     return TEXT("Kemik Yığını");
	case ERawSourceKind::FiberBush:
	default:                           return TEXT("Lif Çalısı");
	}
}

ARawMaterialNode* ARawMaterialNode::Spawn(UWorld* World, const FVector& Ground, ERawSourceKind Kind)
{
	const FSourceLook Look = LookOf(Kind);
	const FTransform Where(FRotator(0.f, FMath::FRand() * 360.f, 0.f), Ground + FVector(0.f, 0.f, 50.f * Look.Scale.Z));
	ARawMaterialNode* Node = World ? World->SpawnActorDeferred<ARawMaterialNode>(ARawMaterialNode::StaticClass(), Where) : nullptr;
	if (Node)
	{
		Node->Kind = Kind;
		Node->FinishSpawning(Where);
	}
	return Node;
}

void ARawMaterialNode::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyKind();
}

void ARawMaterialNode::BeginPlay()
{
	Super::BeginPlay();
	ApplyKind();
	Supply = MaxSupply;
	UpdateLook();
}

void ARawMaterialNode::ApplyKind()
{
	const FSourceLook Look = LookOf(Kind);
	Body->SetStaticMesh(BasicShape(Look.Shape));
	FullScale = Look.Scale;
	Color = Look.Color;
	// Empty drops, or another kind's defaults (Kind changed in the Details panel): this kind's defaults.
	if (Drops.Num() == 0 || IsOtherKindsDefault(Drops, Kind))
	{
		Drops = DefaultDrops(Kind);
	}
	UpdateLook();
}

void ARawMaterialNode::UpdateLook()
{
	const bool bEmpty = IsDepleted();
	Body->SetRelativeScale3D(bEmpty ? FullScale * DepletedScale : FullScale);
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Body->GetMaterial(0));
		if (!Material || Material->Parent != Base)
		{
			Material = UMaterialInstanceDynamic::Create(Base, this);
			Body->SetMaterial(0, Material);
		}
		Material->SetVectorParameterValue(TEXT("Color"), bEmpty ? Color * 0.4f : Color);
	}
}

int32 ARawMaterialNode::Harvest(AForestCharacter* Player, FString& OutGot)
{
	OutGot.Reset();
	if (!Player || IsDepleted())
	{
		return 0;
	}
	int32 Total = 0;
	for (const FRawMaterialDrop& Each : Drops)
	{
		if (Each.Item == EForestItem::None || FMath::FRand() > Each.Probability)
		{
			continue;
		}
		const int32 Count = FMath::RandRange(Each.Min, FMath::Max(Each.Min, Each.Max));
		const int32 Added = Count > 0 ? Player->TestGiveItem(Each.Item, Count) : 0;
		if (Added > 0)
		{
			Total += Added;
			OutGot += FString::Printf(TEXT("%s+%d %s"), OutGot.IsEmpty() ? TEXT("") : TEXT(", "), Added, *ForestItems::GetDisplayName(Each.Item));
		}
	}
	// A full pack leaves the source as it was.
	if (Total == 0 || MaxSupply <= 0)
	{
		return Total;
	}
	--Supply;
	if (IsDepleted() && RespawnSeconds > 0.f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(RegrowTimer, this, &ARawMaterialNode::Regrow, RespawnSeconds, false);
	}
	UpdateLook();
	return Total;
}

void ARawMaterialNode::Regrow()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RegrowTimer);
	}
	Supply = MaxSupply;
	UpdateLook();
}

float ARawMaterialNode::GetRegrowSecondsLeft() const
{
	return GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(RegrowTimer)
		? GetWorld()->GetTimerManager().GetTimerRemaining(RegrowTimer)
		: 0.f;
}

FString ARawMaterialNode::GetPrompt() const
{
	if (IsDepleted())
	{
		return RespawnSeconds > 0.f
			? FString::Printf(TEXT("%s: tükendi  —  %.0f sn sonra yenilenir"), *KindName(Kind), GetRegrowSecondsLeft())
			: FString::Printf(TEXT("%s: tükendi"), *KindName(Kind));
	}
	FString What;
	for (const FRawMaterialDrop& Each : Drops)
	{
		if (Each.Item != EForestItem::None)
		{
			What += (What.IsEmpty() ? TEXT("") : TEXT(" / ")) + ForestItems::GetDisplayName(Each.Item);
		}
	}
	return MaxSupply > 0
		? FString::Printf(TEXT("[E] Topla: %s  (%d / %d)"), *What, Supply, MaxSupply)
		: FString::Printf(TEXT("[E] Topla: %s"), *What);
}
