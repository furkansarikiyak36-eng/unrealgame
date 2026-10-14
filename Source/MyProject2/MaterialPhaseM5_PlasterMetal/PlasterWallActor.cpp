// PlasterWallActor.cpp
#include "PlasterWallActor.h"
#include "PlasterComponent.h"
#include "PlasterDefinition.h"
#include "Components/StaticMeshComponent.h"

APlasterWallActor::APlasterWallActor()
{
    PrimaryActorTick.bCanEverTick = false;
    WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall"));
    RootComponent = WallMesh;
    PlasterComponent = CreateDefaultSubobject<UPlasterComponent>(TEXT("Plaster"));
}

void APlasterWallActor::InitializeWall(float Thickness)
{
    if (PlasterComponent)
    {
        UPlasterDefinition* Def = NewObject<UPlasterDefinition>(this);
        PlasterComponent->ApplyPlaster(Def, Thickness);
    }
}

void APlasterWallActor::UpdatePlaster(float DeltaHours, float WaterIntensity)
{
    if (!PlasterComponent) return;
    PlasterComponent->Cure(DeltaHours);
    if (WaterIntensity > 0.0f)
        PlasterComponent->ApplyWaterDamage(WaterIntensity, DeltaHours);
}
