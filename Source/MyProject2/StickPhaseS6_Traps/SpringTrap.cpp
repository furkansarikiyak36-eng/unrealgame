#include "SpringTrap.h"

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "StickPhaseS6_Traps/TrapMechanismComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

ASpringTrap::ASpringTrap()
{
	PrimaryActorTick.bCanEverTick = false;

	// Give the actor a real root before attaching any child components.
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;
	BaseMesh->SetSimulatePhysics(false);

	SpringArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(BaseMesh);
	SpringArm->SetSimulatePhysics(false);

	SpringConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("SpringConstraint"));
	SpringConstraint->SetupAttachment(BaseMesh);

	Mechanism = CreateDefaultSubobject<UTrapMechanismComponent>(TEXT("Mechanism"));
}

void ASpringTrap::SetSpringTension(float TensionAmount)
{
	if (!FMath::IsFinite(TensionAmount))
	{
		return;
	}

	Tension = FMath::Clamp(TensionAmount, 0.0f, 1.0f);
}

void ASpringTrap::ReleaseSpring(AActor* Victim)
{
	if (!IsValid(Victim) || !Mechanism ||
		!FMath::IsFinite(Tension) || Tension <= 0.0f ||
		!FMath::IsFinite(LaunchForce) || LaunchForce <= 0.0f)
	{
		return;
	}

	ACharacter* CharacterVictim = Cast<ACharacter>(Victim);
	UPrimitiveComponent* PrimitiveVictim = Cast<UPrimitiveComponent>(Victim->GetRootComponent());
	const bool bCanLaunchCharacter = IsValid(CharacterVictim);
	const bool bCanImpulsePhysics = IsValid(PrimitiveVictim) && PrimitiveVictim->IsSimulatingPhysics();

	// Do not consume the trap or apply damage when there is no supported launch target.
	if (!bCanLaunchCharacter && !bCanImpulsePhysics)
	{
		return;
	}

	// The mechanism owns armed/cooldown/consumed state. Physical effects happen only
	// after the mechanism accepted the trigger, preventing cooldown bypass by repeated calls.
	if (!Mechanism->Trigger(Victim))
	{
		return;
	}

	const float AppliedLaunch = LaunchForce * Tension;
	const FVector LaunchVelocity = FVector::UpVector * AppliedLaunch;
	if (bCanLaunchCharacter)
	{
		CharacterVictim->LaunchCharacter(LaunchVelocity, true, true);
	}
	else
	{
		PrimitiveVictim->AddImpulse(LaunchVelocity, NAME_None, true);
	}
}
