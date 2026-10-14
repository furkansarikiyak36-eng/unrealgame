#include "ChemistryLayer.h"
void UChemistryLayer::SetToxin(int32 K,float V){V=FMath::Clamp(V,0.0f,1.0f);if(V<=0){Cells.Remove(K);return;}Cells.FindOrAdd(K).ToxinConcentration=V;}
float UChemistryLayer::GetToxin(int32 K) const{const FChemistryCell* C=Cells.Find(K);return C?C->ToxinConcentration:0.0f;}
void UChemistryLayer::RemoveCleanCells(float T){for(auto It=Cells.CreateIterator();It;++It)if(It.Value().ToxinConcentration<T&&It.Value().Pollution<T&&It.Value().Radioactivity<T)It.RemoveCurrent();}
