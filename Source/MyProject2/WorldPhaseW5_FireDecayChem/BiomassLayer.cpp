#include "BiomassLayer.h"
void UBiomassLayer::SetCell(int32 K,const FBiomassCell& C){Cells.Add(K,C);RecomputeTotal();}
FBiomassCell UBiomassLayer::GetCell(int32 K) const{const FBiomassCell* C=Cells.Find(K);return C?*C:FBiomassCell();}
void UBiomassLayer::RecomputeTotal(){TotalBiomass=0;for(const auto& P:Cells)TotalBiomass+=FMath::Max(0.0f,P.Value.Biomass);}
