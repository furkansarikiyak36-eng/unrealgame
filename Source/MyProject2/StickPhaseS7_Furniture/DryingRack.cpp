#include "DryingRack.h"
#include "Components/StaticMeshComponent.h"
ADryingRack::ADryingRack(){PrimaryActorTick.bCanEverTick=true;RackMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rack"));RootComponent=RackMesh;}
bool ADryingRack::HangItem(FName N){if(N.IsNone()||HangingItems.Num()>=MaxSlots)return false;FDryingItem I;I.ItemName=N;HangingItems.Add(I);return true;} FName ADryingRack::CollectItem(int32 I){if(!HangingItems.IsValidIndex(I)||!HangingItems[I].bReady)return NAME_None;FName N=HangingItems[I].ItemName;HangingItems.RemoveAt(I);return N;} void ADryingRack::Tick(float D){Super::Tick(D);for(auto&I:HangingItems)if(!I.bReady){I.Progress+=D/FMath::Max(DryingTime,.01f);I.bReady=I.Progress>=1.f;}}
