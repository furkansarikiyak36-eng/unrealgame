#include "Phase6_Utility/TrapActor.h"

#include "Components/BoxComponent.h"
#include "StickPhaseS6_Traps/TrapMechanismComponent.h"
#include "GameFramework/Actor.h"

ATrapActor::ATrapActor()
{
	PrimaryActorTick.bCanEverTick = false;
	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = TriggerVolume;
	TriggerVolume->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ATrapActor::HandleOverlap);

	Mechanism = CreateDefaultSubobject<UTrapMechanismComponent>(TEXT("Mechanism"));
}

void ATrapActor::Trigger(AActor* Victim)
{
	if (!Mechanism)
	{
		return;
	}
	Mechanism->Damage = FMath::Max(0.f, Damage);
	Mechanism->Cooldown = FMath::Max(0.f, Cooldown);
	Mechanism->Trigger(Victim);
}

void ATrapActor::Arm()
{
	if (Mechanism) Mechanism->Arm();
}

void ATrapActor::Disarm()
{
	if (Mechanism) Mechanism->Disarm();
}

void ATrapActor::HandleOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (IsValid(OtherActor) && OtherActor != this)
	{
		Trigger(OtherActor);
	}
}
