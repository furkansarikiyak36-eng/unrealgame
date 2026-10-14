#include "Phase6_Utility/RaftActor.h"
#include "Phase1_Physical/LogActor.h"
#include "Phase2_Transport/LogBuoyancyComponent.h"
#include "Components/StaticMeshComponent.h"
ARaftActor::ARaftActor(){PrimaryActorTick.bCanEverTick=false;RaftMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Raft"));RootComponent=RaftMesh;RaftMesh->SetSimulatePhysics(true);}
bool ARaftActor::AssembleFromLogs(const TArray<ALogActor*>& Logs){if(Logs.Num()<6)return false;int32 I=0;for(ALogActor* L:Logs){if(!L||!L->SetLogState(ELogState::Installed))return false;L->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);L->SetActorRelativeLocation(FVector((I%6)*50.0f,(I/6)*40.0f,0));++I;}return true;}
void ARaftActor::Steer(float Y){if(UPrimitiveComponent* P=Cast<UPrimitiveComponent>(RootComponent))P->AddTorqueInRadians(FVector(0,0,Y*5000.0f),NAME_None,true);}
