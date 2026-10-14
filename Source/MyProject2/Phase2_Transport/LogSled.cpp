#include "Phase2_Transport/LogSled.h"
#include "Phase1_Physical/LogActor.h"
#include "Components/StaticMeshComponent.h"
ALogSled::ALogSled(){PrimaryActorTick.bCanEverTick=false;Platform=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform"));RootComponent=Platform;Platform->SetSimulatePhysics(true);}
bool ALogSled::LoadLog(ALogActor* L){if(!L||LoadedLogs.Num()>=Capacity||!L->SetLogState(ELogState::Installed))return false;const int32 I=LoadedLogs.Num();L->AttachToComponent(Platform,FAttachmentTransformRules::KeepWorldTransform);L->SetActorRelativeLocation(FVector(I*35.0f,0,20.0f));LoadedLogs.Add(L);return true;}
void ALogSled::SetDragForce(float F){if(UPrimitiveComponent* P=Cast<UPrimitiveComponent>(RootComponent))P->AddForce(GetActorForwardVector()*F);}
