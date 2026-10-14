#include "SpawnPositionFinder.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "CollisionQueryParams.h"
bool USpawnPositionFinder::IsOnNavMesh(UWorld* W,const FVector&P) const{if(!W)return false;if(UNavigationSystemV1*N=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W)){FNavLocation O;return N->ProjectPointToNavigation(P,O,FVector(200,200,400));}return false;}
bool USpawnPositionFinder::HasClearance(UWorld*W,const FVector&P) const{return W&&!W->OverlapAnyTestByChannel(P,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(FMath::Max(1.f,ClearanceRadius)),FCollisionQueryParams());}
bool USpawnPositionFinder::IsPositionValid(UWorld*W,const FVector&P) const{return IsOnNavMesh(W,P)&&HasClearance(W,P);}
FVector USpawnPositionFinder::FindValidSpawnPosition(UWorld*W,const FVector&T){if(!W)return FVector::ZeroVector;for(int32 I=0;I<24;++I){const float A=FMath::FRandRange(0.f,2.f*PI),D=FMath::FRandRange(FMath::Min(MinDistance,MaxDistance),FMath::Max(MinDistance,MaxDistance));const FVector C=T+FVector(FMath::Cos(A)*D,FMath::Sin(A)*D,0);FHitResult H;FCollisionQueryParams P;P.AddIgnoredActor(GetTypedOuter<AActor>());if(W->LineTraceSingleByChannel(H,C+FVector(0,0,1000),C-FVector(0,0,2000),ECC_WorldStatic,P)){const FVector G=H.ImpactPoint+FVector(0,0,90);if(IsPositionValid(W,G))return G;}}return FVector::ZeroVector;}
