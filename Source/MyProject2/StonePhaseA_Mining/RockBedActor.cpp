#include "RockBedActor.h"
#include "StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

ARockBedActor::ARockBedActor()
{
    PrimaryActorTick.bCanEverTick = false;
    RockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rock"));
    RootComponent = RockMesh;
    RockMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

int32 ARockBedActor::MineChunk(float Strength, AActor* InstigatorActor)
{
    if (RemainingChunks <= 0 || Strength <= 0.0f || !GetWorld()) return 0;
    const int32 Chunks = FMath::Clamp(FMath::RoundToInt(Strength), 1, RemainingChunks);
    const FVector Origin = GetActorLocation();
    for (int32 i = 0; i < Chunks; ++i)
    {
        const FVector Offset(FMath::RandRange(-40.f, 40.f), FMath::RandRange(-40.f, 40.f), 30.f);
        if (SpawnStoneClass)
        {
            if (AStoneActor* Stone = GetWorld()->SpawnActor<AStoneActor>(SpawnStoneClass, Origin + Offset, FRotator::ZeroRotator))
                if (StoneDefinition) Stone->InitializeFromDefinition(StoneDefinition);
        }
    }
    RemainingChunks -= Chunks;
    if (RemainingChunks <= 0) SetLifeSpan(5.0f);
    return Chunks;
}
