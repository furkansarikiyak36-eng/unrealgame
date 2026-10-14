#include "EdgeCostCalculator.h"
float UEdgeCostCalculator::ComputeEdgeCost(const FVector&A,const FVector&B,float D)const{return FVector::Distance(A,B)*FMath::Max(0.f,MovementWeight)+FMath::Abs(A.Z-B.Z)*FMath::Max(0.f,TerrainWeight)+FMath::Max(0.f,D)*FMath::Max(0.f,DangerWeight);}
