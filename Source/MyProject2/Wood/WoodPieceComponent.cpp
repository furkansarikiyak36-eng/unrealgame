#include "WoodPieceComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

UWoodPieceComponent::UWoodPieceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UPrimitiveComponent* UWoodPieceComponent::GetPrimitive() const
{
	if (CachedPrimitive.IsValid())
	{
		return CachedPrimitive.Get();
	}

	if (const AActor* Owner = GetOwner())
	{
		UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		if (!Prim)
		{
			Prim = Owner->FindComponentByClass<UPrimitiveComponent>();
		}
		CachedPrimitive = Prim;
		return Prim;
	}
	return nullptr;
}

void UWoodPieceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UPrimitiveComponent* Prim = GetPrimitive())
	{
		Prim->SetNotifyRigidBodyCollision(true);
		Prim->OnComponentHit.AddDynamic(this, &UWoodPieceComponent::HandleHit);
	}
}

void UWoodPieceComponent::ChangeState(EWoodState NewState)
{
	if (NewState == State)
	{
		return;
	}
	const EWoodState Old = State;
	State = NewState;
	OnStateChanged.Broadcast(this, Old, NewState);
}

bool UWoodPieceComponent::PickUp(USceneComponent* AttachTo, FName Socket)
{
	UPrimitiveComponent* Prim = GetPrimitive();
	if (!Prim || !AttachTo || State == EWoodState::Installed)
	{
		return false;
	}

	Prim->SetSimulatePhysics(false);
	Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Prim->AttachToComponent(AttachTo, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
	ChangeState(EWoodState::Carried);
	return true;
}

bool UWoodPieceComponent::Drop()
{
	UPrimitiveComponent* Prim = GetPrimitive();
	if (!Prim || State != EWoodState::Carried)
	{
		return false;
	}

	Prim->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Prim->SetCollisionEnabled(LooseCollision);
	Prim->SetSimulatePhysics(true);
	ChangeState(EWoodState::Loose);
	return true;
}

bool UWoodPieceComponent::ThrowPiece(FVector Velocity)
{
	if (!Drop())
	{
		return false;
	}
	if (UPrimitiveComponent* Prim = GetPrimitive())
	{
		Prim->SetPhysicsLinearVelocity(Velocity);
	}
	return true;
}

void UWoodPieceComponent::Roll(FVector Direction, float Strength)
{
	UPrimitiveComponent* Prim = GetPrimitive();
	if (!Prim || State != EWoodState::Loose || !Prim->IsSimulatingPhysics())
	{
		return;
	}

	const FVector Dir = Direction.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return;
	}

	Prim->AddImpulse(Dir * Strength, NAME_None, false);
	const FVector Axis = FVector::CrossProduct(FVector::UpVector, Dir);
	Prim->AddAngularImpulseInRadians(Axis * Strength * 0.01f, NAME_None, true);
}

void UWoodPieceComponent::DragToward(FVector TargetLocation)
{
	UPrimitiveComponent* Prim = GetPrimitive();
	if (!Prim || State != EWoodState::Loose || !Prim->IsSimulatingPhysics())
	{
		return;
	}

	const FVector Dir = (TargetLocation - Prim->GetComponentLocation()).GetSafeNormal2D();
	Prim->AddForce(Dir * DragAcceleration, NAME_None, true);
}

void UWoodPieceComponent::Push(FVector Impulse)
{
	UPrimitiveComponent* Prim = GetPrimitive();
	if (!Prim || State != EWoodState::Loose || !Prim->IsSimulatingPhysics())
	{
		return;
	}
	Prim->AddImpulse(Impulse, NAME_None, false);
}

bool UWoodPieceComponent::Install(AActor* Structure, USceneComponent* AttachTo)
{
	UPrimitiveComponent* Prim = GetPrimitive();
	if (!Prim)
	{
		return false;
	}

	if (State == EWoodState::Carried)
	{
		Prim->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	Prim->SetSimulatePhysics(false);
	Prim->SetCollisionEnabled(LooseCollision);

	if (AttachTo)
	{
		Prim->AttachToComponent(AttachTo, FAttachmentTransformRules::KeepWorldTransform);
	}

	InstalledOn = Structure;
	ChangeState(EWoodState::Installed);
	return true;
}

bool UWoodPieceComponent::Uninstall()
{
	UPrimitiveComponent* Prim = GetPrimitive();
	if (!Prim || State != EWoodState::Installed)
	{
		return false;
	}

	Prim->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Prim->SetCollisionEnabled(LooseCollision);
	Prim->SetSimulatePhysics(true);
	InstalledOn.Reset();
	ChangeState(EWoodState::Loose);
	return true;
}

void UWoodPieceComponent::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const float Strength = NormalImpulse.Size();
	if (Strength >= MinImpactImpulse)
	{
		OnImpact.Broadcast(this, OtherActor, Strength);
	}
}
