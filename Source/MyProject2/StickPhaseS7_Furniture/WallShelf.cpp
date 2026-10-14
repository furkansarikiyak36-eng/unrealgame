#include "WallShelf.h"
#include "Components/StaticMeshComponent.h"

AWallShelf::AWallShelf(){PrimaryActorTick.bCanEverTick = false;
 ShelfMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShelfMesh")); RootComponent=ShelfMesh;}
bool AWallShelf::AttachToWall(AActor*W){if(!W)return false;AttachToActor(W,FAttachmentTransformRules::KeepWorldTransform);AttachedWall=W;return true;} bool AWallShelf::PlaceItem(FName N){if(N.IsNone()||Items.Num()>=MaxSlots)return false;Items.Add(N);return true;} FName AWallShelf::TakeItem(int32 I){if(!Items.IsValidIndex(I))return NAME_None;FName N=Items[I];Items.RemoveAt(I);return N;}
