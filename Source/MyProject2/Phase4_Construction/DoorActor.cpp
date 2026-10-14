#include "Phase4_Construction/DoorActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
ADoorActor::ADoorActor(){PrimaryActorTick.bCanEverTick=false;Pivot=CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));RootComponent=Pivot;DoorMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));DoorMesh->SetupAttachment(Pivot);Hinge=CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("Hinge"));Hinge->SetupAttachment(Pivot);}
void ADoorActor::ToggleDoor(){bIsOpen=!bIsOpen;DoorMesh->SetRelativeRotation(FRotator(0,bIsOpen?OpenYaw:0.0f,0));}
