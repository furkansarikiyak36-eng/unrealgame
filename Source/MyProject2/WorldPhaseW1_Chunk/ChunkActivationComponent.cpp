#include "ChunkActivationComponent.h"
#include "ChunkActivationPolicy.h"
#include "WorldCore/ChunkState.h"
#include "WorldCore/ChunkCoordinate.h"
#include "WorldCore/WorldStateSubsystem.h"
UChunkActivationComponent::UChunkActivationComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UChunkActivationComponent::ActivateChunk(UChunkState* C){if(!C)return;const bool Was=C->Metadata.ActivationMode==EChunkActivationMode::Passive;C->Metadata.ActivationMode=EChunkActivationMode::Continuous;C->Metadata.LastSimulationTime=FPlatformTime::Seconds();CurrentMode=EChunkActivationMode::Continuous;if(Was)OnChunkActivated.Broadcast(C->Coordinate);}
void UChunkActivationComponent::DeactivateChunk(UChunkState* C){if(!C)return;C->Metadata.ActivationMode=EChunkActivationMode::Passive;CurrentMode=EChunkActivationMode::Passive;OnChunkDeactivated.Broadcast(C->Coordinate);}
void UChunkActivationComponent::EvaluateActivation(UChunkState* C,const FVector& P){if(!C||!Policy)return;const UWorldStateSubsystem* WS=GetWorld()?GetWorld()->GetSubsystem<UWorldStateSubsystem>():nullptr;const FVector Center=WS?WS->ChunkSettings.ChunkToWorldCenter(C->Coordinate):FVector((C->Coordinate.X+0.5f)*6400.0f,(C->Coordinate.Y+0.5f)*6400.0f,0);const auto New=Policy->DetermineMode(FVector::Dist2D(P,Center)/100.0f);if(New!=C->Metadata.ActivationMode){const auto Old=C->Metadata.ActivationMode;C->Metadata.ActivationMode=New;CurrentMode=New;if(Old==EChunkActivationMode::Passive&&New!=Old)OnChunkActivated.Broadcast(C->Coordinate);if(New==EChunkActivationMode::Passive)OnChunkDeactivated.Broadcast(C->Coordinate);}}
