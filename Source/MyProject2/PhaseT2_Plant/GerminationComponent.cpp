#include "GerminationComponent.h"
#include "PlantDefinition.h"
#include "SimCore/DeterministicRandom.h"
UGerminationComponent::UGerminationComponent(){PrimaryComponentTick.bCanEverTick=false;}
float UGerminationComponent::TempSuitability(float T)const{if(!Definition)return 0.f;return FMath::Clamp(1.f-FMath::Abs(T-Definition->OptimalGerminationTemp)/15.f,0.f,1.f);}
float UGerminationComponent::MoistureSuitability(float W)const{if(W<10.f)return 0.f;if(W<30.f)return(W-10.f)/20.f;if(W<=70.f)return 1.f;if(W<=90.f)return(90.f-W)/20.f;return 0.f;}
float UGerminationComponent::ComputeGerminationProbability(float T,float W,float Q)const{if(!Definition)return 0.f;return FMath::Clamp(Definition->BaseGerminationRate*TempSuitability(T)*MoistureSuitability(W)*FMath::Clamp(Q,0.f,1.f),0.f,1.f);}
int32 UGerminationComponent::RollGermination(int32 Count,float Probability,int32 Seed)const{FDeterministicRandom R{uint32(Seed)};int32 Success=0;for(int32 I=0;I<FMath::Max(0,Count);++I)if(R.Roll(Probability))++Success;return Success;}
