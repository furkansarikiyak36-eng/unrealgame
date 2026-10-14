#include "SparseCellStorage.h"
void USparseCellStorage::SetCellValue(int32 X,int32 Y,float V,float D){const uint64 K=MakeKey(X,Y);if(FMath::IsNearlyEqual(V,D,0.001f)){Records.Remove(K);return;}FSparseCellRecord& R=Records.FindOrAdd(K);R.LocalX=X;R.LocalY=Y;R.Value=V;R.bHasData=true;}
float USparseCellStorage::GetCellValue(int32 X,int32 Y,float D) const{if(const FSparseCellRecord* R=Records.Find(MakeKey(X,Y)))return R->Value;return D;}
bool USparseCellStorage::HasCustomValue(int32 X,int32 Y) const{return Records.Contains(MakeKey(X,Y));}
void USparseCellStorage::RemoveCell(int32 X,int32 Y){Records.Remove(MakeKey(X,Y));}
