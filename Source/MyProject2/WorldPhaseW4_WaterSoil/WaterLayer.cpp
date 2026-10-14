#include "WaterLayer.h"
void UWaterLayer::SetCell(int32 K,const FWaterCell& C){Cells.Add(K,C);RecomputeTotals();}
FWaterCell UWaterLayer::GetCell(int32 K) const{const FWaterCell* C=Cells.Find(K);return C?*C:FWaterCell();}
void UWaterLayer::RecomputeTotals(){TotalVolume=0;AverageLevel=0;int32 N=0;for(const auto& P:Cells){TotalVolume+=FMath::Max(0.0f,P.Value.Volume);AverageLevel+=P.Value.Level;++N;}if(N)AverageLevel/=N;}
