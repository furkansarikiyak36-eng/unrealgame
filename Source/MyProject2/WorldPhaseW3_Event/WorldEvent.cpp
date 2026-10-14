#include "WorldEvent.h"
FWorldEvent UWorldEventLibrary::CreateEvent(EWorldEventType T,const FVector& P,float R,float M,FName S){FWorldEvent E;E.Type=T;E.WorldLocation=P;E.AffectedRadius=FMath::Max(0.0f,R);E.Magnitude=M;E.SourceTag=S;E.Timestamp=FPlatformTime::Seconds();E.ChunkCoord=FIntPoint(FMath::FloorToInt(P.X/6400.0f),FMath::FloorToInt(P.Y/6400.0f));return E;}
bool UWorldEventLibrary::IsInRadius(const FWorldEvent& E,const FVector& P){return FVector::Dist(E.WorldLocation,P)<=E.AffectedRadius*100.0f;}
