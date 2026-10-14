#include "WattleFenceComponent.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "StickCore/StickConnectionGraph.h"
#include "Engine/World.h"
UWattleFenceComponent::UWattleFenceComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UWattleFenceComponent::BuildFence(const TArray<FVector>& P,int32 Rows){if(P.Num()<2||Rows<=0||!StickClass)return false;TArray<AStickActor*> Posts;for(const FVector&L:P)if(auto*S=GetWorld()->SpawnActor<AStickActor>(StickClass,L,FRotator::ZeroRotator))Posts.Add(S);for(int32 R=0;R<Rows;++R)for(int32 i=0;i<Posts.Num()-1;++i){FVector A=Posts[i]->GetActorLocation()+FVector(0,0,(R+1)*PostHeight/(Rows+1)),B=Posts[i+1]->GetActorLocation()+FVector(0,0,(R+1)*PostHeight/(Rows+1));if(auto*S=GetWorld()->SpawnActor<AStickActor>(StickClass,(A+B)*.5f,(B-A).Rotation()))if(auto*G=Posts[i]->FindComponentByClass<UStickConnectionGraph>())G->ConnectWoven(S,TEXT("Weave"),TEXT("Post"));}return Posts.Num()>=2;}
bool UWattleFenceComponent::AddClayFill(AStickActor*S){return S!=nullptr;}
