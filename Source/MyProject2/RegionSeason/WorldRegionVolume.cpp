#include "WorldRegionVolume.h"
#include "Components/BoxComponent.h"
#include "RegionSeasonSubsystem.h"
#include "Engine/World.h"

AWorldRegionVolume::AWorldRegionVolume()
{
    PrimaryActorTick.bCanEverTick = false;
    RegionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("RegionBounds"));
    SetRootComponent(RegionBounds);
    RegionBounds->SetBoxExtent(FVector(5000.0f, 5000.0f, 2000.0f));
    RegionBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RegionBounds->SetGenerateOverlapEvents(false);
    RegionBounds->SetHiddenInGame(true);
    RegionBounds->ShapeColor = FColor(70, 160, 90);
    RegionBounds->bDrawOnlyIfSelected = true;
}

bool AWorldRegionVolume::ContainsWorldPoint(const FVector& WorldPoint) const
{
    if (!RegionBounds) return false;
    const FVector Local = RegionBounds->GetComponentTransform().InverseTransformPosition(WorldPoint);
    const FVector Extent = RegionBounds->GetUnscaledBoxExtent();
    return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

void AWorldRegionVolume::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
        if (URegionSeasonSubsystem* Subsystem = World->GetSubsystem<URegionSeasonSubsystem>())
            Subsystem->RegisterRegion(this);
}

void AWorldRegionVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
        if (URegionSeasonSubsystem* Subsystem = World->GetSubsystem<URegionSeasonSubsystem>())
            Subsystem->UnregisterRegion(this);
    Super::EndPlay(EndPlayReason);
}
