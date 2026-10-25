#include "Phase6_Utility/TrapActor.h"

#include "CarvableLog.h"
#include "Components/BoxComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "StickPhaseS6_Traps/TrapMechanismComponent.h"

ATrapActor::ATrapActor()
{
	PrimaryActorTick.bCanEverTick = false;
	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = TriggerVolume;
	TriggerVolume->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ATrapActor::HandleOverlap);

	Mechanism = CreateDefaultSubobject<UTrapMechanismComponent>(TEXT("Mechanism"));
	Mechanism->OnTrapTriggered.AddDynamic(this, &ATrapActor::HandleMechanismTriggered);
}

void ATrapActor::Trigger(AActor* Victim)
{
	if (!Mechanism || !FMath::IsFinite(Damage) || Damage < 0.f
		|| !FMath::IsFinite(Cooldown) || Cooldown < 0.f)
	{
		return;
	}
	Mechanism->Damage = Damage;
	Mechanism->Cooldown = Cooldown;
	Mechanism->bConsumeOnTrigger = bConsumeAfterPayloadRelease
		&& (TrapType == ETrapType::RollingLog || TrapType == ETrapType::FallingLog);
	Mechanism->Trigger(Victim);
}

void ATrapActor::Arm()
{
	if (Mechanism && !bPayloadReleased)
	{
		Mechanism->Arm();
	}
	else if (Mechanism && TrapType == ETrapType::SpikePit)
	{
		Mechanism->Arm();
	}
}

void ATrapActor::Disarm()
{
	if (Mechanism) Mechanism->Disarm();
}

void ATrapActor::HandleOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32, bool, const FHitResult&)
{
	if (!IsValid(OtherActor) || OtherActor == this || !Mechanism)
	{
		return;
	}

	// Pressure/proximity volumes are automatic. Tripwire and manual traps need an explicit external signal.
	if (Mechanism->TriggerType == ETrapTriggerType::Pressure)
	{
		if (OtherActor->IsA<ACharacter>() || (OtherComponent && OtherComponent->IsSimulatingPhysics()))
		{
			Trigger(OtherActor);
		}
	}
	else if (Mechanism->TriggerType == ETrapTriggerType::Proximity && OtherActor->IsA<ACharacter>())
	{
		Trigger(OtherActor);
	}
}

void ATrapActor::HandleMechanismTriggered(AActor* Victim)
{
	switch (TrapType)
	{
	case ETrapType::RollingLog:
	case ETrapType::FallingLog:
		if (!bPayloadReleased && IsValid(PayloadLog))
		{
			bPayloadReleased = true;
			PayloadLog->LetFall();
			if (TrapType == ETrapType::RollingLog && PayloadImpulse > 0.f
				&& PayloadLog->MeshComponent && PayloadLog->MeshComponent->IsSimulatingPhysics())
			{
				PayloadLog->MeshComponent->AddImpulse(GetActorForwardVector() * PayloadImpulse, NAME_None, true);
			}
		}
		break;
	case ETrapType::SpikePit:
		if (ACharacter* Character = Cast<ACharacter>(Victim))
		{
			Character->LaunchCharacter(FVector(0.f, 0.f, FMath::Max(0.f, SpikeLaunchVelocity)),
				true, true);
		}
		break;
	default:
		break;
	}
}
