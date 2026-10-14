#include "RaidStageSelector.h"
#include "RaidStageTableAsset.h"
int32 URaidStageSelector::BinarySearchThreshold(int32 Score,const TArray<int32>& T){int32 L=0,H=T.Num()-1,R=-1;while(L<=H){const int32 M=L+(H-L)/2;if(T[M]<=Score){R=T[M];L=M+1;}else H=M-1;}return R;}
int32 URaidStageSelector::FindNearestStage(int32 Score,const URaidStageTableAsset* Table){if(!Table||Table->Stages.IsEmpty())return 0;TArray<FRaidStageConfig> S=Table->Stages;S.Sort([](const auto&A,const auto&B){return A.Threshold<B.Threshold;});int32 Result=S[0].StageIndex;for(const auto& C:S){if(C.Threshold>Score)break;Result=C.StageIndex;}return Result;}
