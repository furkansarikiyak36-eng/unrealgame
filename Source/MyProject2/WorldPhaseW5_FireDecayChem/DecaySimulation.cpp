#include "DecaySimulation.h"
#include "WorldCore/ChunkState.h"
#include "WorldPhaseW2_Lazy/ExponentialDecayModel.h"
void UDecaySimulation::SimulateChunk(UChunkState* C,float Days,float T){if(!C||Days<=0)return;for(FChunkCell& Cell:C->Cells){FBiomassCell& B=Cell.Biomass;if(B.Biomass<=0||B.bIsBurning)continue;const float K=UExponentialDecayModel::ComputeEffectiveRate(BaseDecayRate*FMath::Max(0.0f,B.DecayRate),B.MoistureContent,T);const float Old=B.Biomass;B.Biomass=UExponentialDecayModel::ComputeDecay(B.Biomass,K,Days);if(Old>B.Biomass)B.Health=FMath::Clamp(B.Health-K*Days,0.0f,1.0f);if(B.Biomass<0.01f){B.Biomass=0;B.Health=0;B.SpeciesID=NAME_None;B.bIsBurning=false;}Cell.bIsDirty=true;}C->MarkDirty();}
