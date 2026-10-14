#include "StakeWallComponent.h"
#include "StakeActor.h"
#include "Engine/World.h"
UStakeWallComponent::UStakeWallComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UStakeWallComponent::BuildStakeWall(const FVector&A,const FVector&B,int32 N){if(N<=0||!StakeClass)return false;FVector D=B-A;float L=D.Size();D=D.GetSafeNormal();for(int32 i=0;i<N;++i){if(auto*S=GetWorld()->SpawnActor<AStakeActor>(StakeClass,A+D*(N==1?L*.5f:L*i/(N-1)),FRotator(0,D.Rotation().Yaw,0))){S->Sharpen();Stakes.Add(S);}}return Stakes.Num()>0;}
