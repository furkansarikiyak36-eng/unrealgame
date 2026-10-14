#include "Phase6_Utility/TrapActor.h"
#include "Components/BoxComponent.h"
ATrapActor::ATrapActor(){PrimaryActorTick.bCanEverTick=false;TriggerVolume=CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));RootComponent=TriggerVolume;TriggerVolume->SetBoxExtent(FVector(100,100,100));TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);TriggerVolume->SetGenerateOverlapEvents(true);}
void ATrapActor::Trigger(AActor* V){if(!GetWorld()||GetWorld()->GetTimeSeconds()-LastTriggerTime<Cooldown||!V)return;LastTriggerTime=GetWorld()->GetTimeSeconds();/* Game-specific damage interface should be invoked here. */}
