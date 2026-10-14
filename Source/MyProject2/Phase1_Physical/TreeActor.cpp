#include "Phase1_Physical/TreeActor.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
#include "Components/StaticMeshComponent.h"
ATreeActor::ATreeActor(){PrimaryActorTick.bCanEverTick=false;TrunkMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trunk"));RootComponent=TrunkMesh;TrunkMesh->SetCollisionProfileName(TEXT("BlockAll"));}
void ATreeActor::ApplyAxeHit(float Strength,AActor* InstigatorActor){if(bFelled)return;CurrentHits+=FMath::Max(1,FMath::RoundToInt(Strength));if(CurrentHits>=FMath::Max(1,Profile.HitsToFell)){FVector D=InstigatorActor?(GetActorLocation()-InstigatorActor->GetActorLocation()).GetSafeNormal():GetActorForwardVector();FellTree(D);}}
void ATreeActor::FellTree(const FVector& FallDirection)
{
    if(bFelled)return;bFelled=true;ULogDefinition* Def=HarvestLogDefinition.LoadSynchronous();
    if(Def&&GetWorld())
    {
        const int32 Count=FMath::Max(1,FMath::FloorToInt(Profile.TrunkHeight/FMath::Max(Profile.ModuleLength,1.0f)));
        const FVector Axis=FallDirection.GetSafeNormal().IsNearlyZero()?GetActorForwardVector():FallDirection.GetSafeNormal();
        for(int32 I=0;I<Count;++I){const FVector Loc=GetActorLocation()+Axis*(I*Profile.ModuleLength+Profile.ModuleLength*0.5f);FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            if(ALogActor* Log=GetWorld()->SpawnActor<ALogActor>(ALogActor::StaticClass(),Loc,GetActorRotation(),P))Log->InitializeFromDefinition(Def);}
    }
    TrunkMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);SetLifeSpan(1.0f);
}
