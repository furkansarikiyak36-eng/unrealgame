#include "PathInvalidationDetector.h"
#include "Engine/World.h"
bool UPathInvalidationDetector::IsPathStillValid(UWorld*W,const TArray<FVector>&Path)const{if(!W||Path.Num()<2)return false;FCollisionQueryParams P;for(int32 I=0;I<Path.Num()-1;++I){FHitResult H;if(W->LineTraceSingleByChannel(H,Path[I]+FVector(0,0,40),Path[I+1]+FVector(0,0,40),ECC_WorldStatic,P))return false;}return true;}
void UPathInvalidationDetector::RequestPathRecalculation(AActor*E){if(E)PendingRecalculation.Add(E);}
bool UPathInvalidationDetector::ShouldRecalculate(AActor*E,float Dt){if(!E)return false;return PendingRecalculation.Contains(E)||FMath::FRand()<FMath::Clamp(Dt*.5f,0.f,1.f);}
