#include "Phase2_Transport/LogCart.h"
#include "Phase1_Physical/LogActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
ALogCart::ALogCart(){PrimaryActorTick.bCanEverTick=false;Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));RootComponent=Body;WheelL=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelL"));WheelL->SetupAttachment(Body);WheelR=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelR"));WheelR->SetupAttachment(Body);StorageVolume=CreateDefaultSubobject<UBoxComponent>(TEXT("Storage"));StorageVolume->SetupAttachment(Body);StorageVolume->SetBoxExtent(FVector(100,60,40));}
bool ALogCart::LoadLog(ALogActor* L){if(!L||LoadedLogs.Num()>=Capacity||L->GetLogState()==ELogState::Destroyed)return false;if(!L->SetLogState(ELogState::Installed))return false;const int32 I=LoadedLogs.Num();L->AttachToComponent(StorageVolume,FAttachmentTransformRules::KeepWorldTransform);L->SetActorRelativeLocation(FVector((I%3-1)*45.0f,(I/3)*35.0f,20.0f));LoadedLogs.Add(L);return true;}
bool ALogCart::UnloadLog(){while(LoadedLogs.Num()){ALogActor* L=LoadedLogs.Pop();if(L){L->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);return L->SetLogState(ELogState::Loose);}}return false;}
