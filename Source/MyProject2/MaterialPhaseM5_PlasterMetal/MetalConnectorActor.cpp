// MetalConnectorActor.cpp
#include "MetalConnectorActor.h"
#include "CorrosionComponent.h"
#include "Components/StaticMeshComponent.h"

AMetalConnectorActor::AMetalConnectorActor()
{
    PrimaryActorTick.bCanEverTick = false;
    ConnectorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Connector"));
    RootComponent = ConnectorMesh;
    CorrosionComponent = CreateDefaultSubobject<UCorrosionComponent>(TEXT("Corrosion"));
}

void AMetalConnectorActor::InitializeConnector(UMetalDefinition* Def)
{
    if (CorrosionComponent) CorrosionComponent->InitializeFromDefinition(Def);
}

void AMetalConnectorActor::UpdateCorrosion(float Moisture, float DeltaDays)
{
    if (CorrosionComponent) CorrosionComponent->UpdateCorrosion(Moisture, DeltaDays);
}
