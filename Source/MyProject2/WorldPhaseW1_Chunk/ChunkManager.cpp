#include "ChunkManager.h"
#include "ChunkActivationPolicy.h"
#include "WorldCore/WorldStateSubsystem.h"
#include "WorldCore/ChunkStateStore.h"
#include "WorldPhaseW1_Chunk/WorldChunkSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
UChunkManager::UChunkManager(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=1.0f;}
void UChunkManager::TickComponent(float D,ELevelTick T,FActorComponentTickFunction* F){Super::TickComponent(D,T,F);UWorld* W=GetWorld();if(!W)return;APlayerController* PC=W->GetFirstPlayerController();APawn* P=PC?PC->GetPawn():nullptr;if(P)UpdateChunkActivations(P->GetActorLocation());}
void UChunkManager::UpdateChunkActivations(const FVector& P){UWorld* W=GetWorld();UWorldStateSubsystem* S=W?W->GetSubsystem<UWorldStateSubsystem>():nullptr;if(!S||!S->StateStore)return;if(UWorldChunkSubsystem* ChunkSys=W->GetSubsystem<UWorldChunkSubsystem>())ChunkSys->ActivateChunksInRadius(P,ActivationRadius);UChunkActivationPolicy* ActivePolicy=Policy;if(!ActivePolicy){ActivePolicy=NewObject<UChunkActivationPolicy>(this);Policy=ActivePolicy;}UWorldChunkSubsystem* ChunkSys=W->GetSubsystem<UWorldChunkSubsystem>();TArray<UChunkState*> Candidates;if(ChunkSys){for(const FIntPoint& Coord:ChunkSys->LoadedChunks)if(UChunkState* Found=S->GetChunk(Coord))Candidates.Add(Found);}else Candidates=S->StateStore->GetAllChunks();for(UChunkState* C:Candidates){if(!C)continue;const FVector Center=S->ChunkSettings.ChunkToWorldCenter(C->Coordinate);const EChunkActivationMode NewMode=ActivePolicy->DetermineMode(FVector::Dist2D(P,Center)/100.0f);// Do not simulate a returning chunk here: the central WorldStateSubsystem performs catch-up
// under its per-frame budget on the next scheduled pass.
C->Metadata.ActivationMode=NewMode;}}
int32 UChunkManager::GetActiveChunkCount() const{UWorld* W=GetWorld();const UWorldStateSubsystem* S=W?W->GetSubsystem<UWorldStateSubsystem>():nullptr;const UWorldChunkSubsystem* CS=W?W->GetSubsystem<UWorldChunkSubsystem>():nullptr;if(!S||!S->StateStore)return 0;int32 N=0;if(CS){for(const FIntPoint& Coord:CS->LoadedChunks)if(const UChunkState* C=S->GetChunk(Coord))if(C->Metadata.ActivationMode==EChunkActivationMode::Continuous)++N;}else{for(UChunkState* C:S->StateStore->GetAllChunks())if(C&&C->Metadata.ActivationMode==EChunkActivationMode::Continuous)++N;}return N;}
