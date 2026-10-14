#include "RaidDifficultyScorer.h"
int32 URaidDifficultyScorer::ComputeSingleScore(int32 L,float S,float D,float M){return FMath::FloorToInt(FMath::Max(0.f,((float)L+S-D)*M));}
int32 URaidDifficultyScorer::ComputePartyScore(const TArray<int32>& Scores,float W0,float R){float Sum=0.f;for(int32 I=0;I<Scores.Num();++I)Sum+=(float)Scores[I]*W0*FMath::Pow(R,(float)I);return FMath::FloorToInt(FMath::Max(0.f,Sum));}
int32 URaidDifficultyScorer::ApplyMultiplierChain(int32 Base,const TArray<FRaidMultiplier>& Chain){TArray<FRaidMultiplier> S=Chain;S.Sort([](const auto&A,const auto&B){return A.Order<B.Order;});float V=(float)Base;for(const auto& M:S)V*=M.Value;return FMath::FloorToInt(FMath::Max(0.f,V));}
