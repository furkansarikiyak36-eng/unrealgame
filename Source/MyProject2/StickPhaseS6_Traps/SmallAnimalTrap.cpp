#include "SmallAnimalTrap.h"
#include "Components/StaticMeshComponent.h"
#include "StickPhaseS6_Traps/TrapMechanismComponent.h"
#include "Components/BoxComponent.h"

ASmallAnimalTrap::ASmallAnimalTrap(){PrimaryActorTick.bCanEverTick = false;
 TrapMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrapMesh")); RootComponent=TrapMesh; TriggerVolume=CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume")); TriggerVolume->SetupAttachment(RootComponent); Mechanism=CreateDefaultSubobject<UTrapMechanismComponent>(TEXT("Mechanism"));}
#include "StickPhaseS6_Traps/TrapMechanismComponent.h"
bool ASmallAnimalTrap::CatchAnimal(AActor*A){if(!A||CaughtAnimal)return false;CaughtAnimal=A;if(Mechanism)Mechanism->Trigger(A);return true;} AActor* ASmallAnimalTrap::ReleaseAnimal(){AActor*A=CaughtAnimal;CaughtAnimal=nullptr;return A;} void ASmallAnimalTrap::ResetTrap(){CaughtAnimal=nullptr;if(Mechanism)Mechanism->Arm();}
