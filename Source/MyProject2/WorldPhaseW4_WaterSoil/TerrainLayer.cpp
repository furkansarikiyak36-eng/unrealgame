#include "TerrainLayer.h"
void UTerrainLayer::DeformCell(int32 K,float D){FTerrainCell& C=Cells.FindOrAdd(K);C.Deformation+=D;C.Height=C.OriginalHeight+C.Deformation;}
float UTerrainLayer::GetHeight(int32 K) const{const FTerrainCell* C=Cells.Find(K);return C?C->Height:0.0f;}
bool UTerrainLayer::IsDeformed(int32 K) const{const FTerrainCell* C=Cells.Find(K);return C&&FMath::Abs(C->Deformation)>0.01f;}
