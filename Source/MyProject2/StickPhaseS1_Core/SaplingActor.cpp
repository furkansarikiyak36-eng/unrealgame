#include "SaplingActor.h"
#include "StickActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
ASaplingActor::ASaplingActor() { PrimaryActorTick.bCanEverTick = false; SaplingMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sapling")); RootComponent=SaplingMesh; SaplingMesh->SetCollisionProfileName(TEXT("BlockAll")); }
int32 ASaplingActor::Harvest(float Strength, AActor*)
{
    if (RemainingSticks<=0 || !StickClass || Strength<=0.f) return 0; const int32 N=FMath::Clamp(FMath::RoundToInt(Strength),1,RemainingSticks);
    for(int32 i=0;i<N;++i) { FVector P=GetActorLocation()+FVector(FMath::FRandRange(-20.f,20.f),FMath::FRandRange(-20.f,20.f),5); GetWorld()->SpawnActor<AStickActor>(StickClass,P,FRotator::ZeroRotator); }
    RemainingSticks-=N; if(RemainingSticks<=0) { SaplingMesh->SetVisibility(false); GetWorldTimerManager().SetTimer(RegrowthTimer,this,&ASaplingActor::OnRegrow,RegrowthTime,false); } return N;
}
void ASaplingActor::OnRegrow() { RemainingSticks=TotalSticks; if(SaplingMesh) SaplingMesh->SetVisibility(true); }
void ASaplingActor::EndPlay(const EEndPlayReason::Type R) { GetWorldTimerManager().ClearTimer(RegrowthTimer); Super::EndPlay(R); }
