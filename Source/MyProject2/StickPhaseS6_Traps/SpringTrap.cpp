#include "SpringTrap.h"
#include "Components/StaticMeshComponent.h"
#include "StickPhaseS6_Traps/TrapMechanismComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

ASpringTrap::ASpringTrap(){PrimaryActorTick.bCanEverTick = false;
 SpringArm=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpringArm")); SpringArm->SetupAttachment(RootComponent); SpringConstraint=CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("SpringConstraint")); SpringConstraint->SetupAttachment(RootComponent); Mechanism=CreateDefaultSubobject<UTrapMechanismComponent>(TEXT("Mechanism"));}
#include "StickPhaseS6_Traps/TrapMechanismComponent.h"
void ASpringTrap::SetSpringTension(float T){Tension=FMath::Clamp(T,0.f,1.f);} void ASpringTrap::ReleaseSpring(AActor* V){if(!V)return;if(auto*P=Cast<UPrimitiveComponent>(V->GetRootComponent()))P->AddImpulse(FVector(0,0,LaunchForce*Tension));if(Mechanism)Mechanism->Trigger(V);}
