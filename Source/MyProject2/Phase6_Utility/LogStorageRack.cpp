#include "Phase6_Utility/LogStorageRack.h"
#include "Phase1_Physical/LogActor.h"
#include "Components/StaticMeshComponent.h"
ALogStorageRack::ALogStorageRack(){PrimaryActorTick.bCanEverTick=false;Frame=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));RootComponent=Frame;}
bool ALogStorageRack::Store(ALogActor* L){if(!L||Stored.Num()>=Capacity||!L->SetLogState(ELogState::Installed))return false;L->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);const int32 I=Stored.Num();L->SetActorRelativeLocation(FVector((I%4)*45.0f,(I/4)*35.0f,25.0f));Stored.Add(L);return true;}
ALogActor* ALogStorageRack::Retrieve(){while(Stored.Num()){ALogActor* L=Stored.Pop();if(L){L->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);L->SetLogState(ELogState::Loose);return L;}}return nullptr;}
