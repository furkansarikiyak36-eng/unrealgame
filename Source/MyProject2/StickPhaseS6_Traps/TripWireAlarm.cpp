#include "TripWireAlarm.h"
#include "Components/StaticMeshComponent.h"
#include "StickPhaseS6_Traps/TrapMechanismComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SplineComponent.h"

ATripWireAlarm::ATripWireAlarm(){PrimaryActorTick.bCanEverTick = false;
 Wire=CreateDefaultSubobject<USplineComponent>(TEXT("Wire")); RootComponent=Wire; TriggerVolume=CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume")); TriggerVolume->SetupAttachment(RootComponent); AlarmLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("AlarmLight")); AlarmLight->SetupAttachment(RootComponent); Mechanism=CreateDefaultSubobject<UTrapMechanismComponent>(TEXT("Mechanism"));}
#include "StickPhaseS6_Traps/TrapMechanismComponent.h"
void ATripWireAlarm::StretchWire(const FVector&A,const FVector&B){SetActorLocation(A);Wire->ClearSplinePoints();Wire->AddSplinePoint(A,ESplineCoordinateSpace::World);Wire->AddSplinePoint(B,ESplineCoordinateSpace::World);} void ATripWireAlarm::TriggerAlarm(AActor* I){if(Mechanism)Mechanism->Trigger(I);AlarmLight->SetIntensity(5000);}
