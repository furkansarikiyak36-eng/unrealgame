#include "WoodCarrierComponent.h"
#include "WoodPieceComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "CollisionQueryParams.h"

UWoodCarrierComponent::UWoodCarrierComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWoodCarrierComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!CarryAttach && GetOwner())
	{
		CarryAttach = GetOwner()->GetRootComponent();
	}
}

float UWoodCarrierComponent::GetCarriedLiters() const
{
	float Sum = 0.f;
	for (const UWoodPieceComponent* P : Carried)
	{
		if (P)
		{
			Sum += P->PieceData.GetVolumeLiters();
		}
	}
	return Sum;
}

bool UWoodCarrierComponent::CanCarry(const UWoodPieceComponent* Piece) const
{
	if (!Piece || !CarryAttach)
	{
		return false;
	}
	return GetCarriedLiters() + Piece->PieceData.GetVolumeLiters() <= MaxCarryLiters;
}

UWoodPieceComponent* UWoodCarrierComponent::FindPieceInView(FVector Start, FVector Direction, float Distance, float Radius) const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WoodCarrierTrace), false, GetOwner());
	const FVector End = Start + Direction.GetSafeNormal() * Distance;

	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Radius), Params))
	{
		if (AActor* Hitted = Hit.GetActor())
		{
			return Hitted->FindComponentByClass<UWoodPieceComponent>();
		}
	}
	return nullptr;
}

bool UWoodCarrierComponent::TryPickUp(UWoodPieceComponent* Piece)
{
	if (!CanCarry(Piece) || Piece->GetState() == EWoodState::Installed)
	{
		return false;
	}

	if (!Piece->PickUp(CarryAttach, CarrySocket))
	{
		return false;
	}

	const int32 Index = Carried.Num();
	Carried.Add(Piece);
	CarriedStorage.Add(Piece);

	if (UPrimitiveComponent* Prim = Piece->GetPrimitive())
	{
		Prim->SetRelativeLocationAndRotation(CarryOffset + StackStep * static_cast<float>(Index), CarryRotation);
	}

	OnCarriedChanged.Broadcast();
	return true;
}

bool UWoodCarrierComponent::DropPiece(UWoodPieceComponent* Piece)
{
	if (!Piece || !Carried.Contains(Piece))
	{
		return false;
	}

	if (!Piece->Drop())
	{
		return false;
	}

	Carried.Remove(Piece);
	CarriedStorage.Remove(Piece);
	OnCarriedChanged.Broadcast();
	return true;
}

bool UWoodCarrierComponent::DropLast()
{
	return Carried.Num() > 0 && DropPiece(Carried.Last());
}

void UWoodCarrierComponent::DropAll()
{
	while (DropLast())
	{
	}
}

bool UWoodCarrierComponent::ThrowLast(FVector Velocity)
{
	if (Carried.Num() == 0)
	{
		return false;
	}

	UWoodPieceComponent* Piece = Carried.Last();
	if (!Piece->ThrowPiece(Velocity))
	{
		return false;
	}

	Carried.Remove(Piece);
	CarriedStorage.Remove(Piece);
	OnCarriedChanged.Broadcast();
	return true;
}
