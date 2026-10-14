#include "Phase2_Transport/LogJackingSystem.h"
#include "Phase1_Physical/LogActor.h"
#include "Engine/World.h"
ULogJackingSystem::ULogJackingSystem(){PrimaryComponentTick.bCanEverTick=false;}
bool ULogJackingSystem::LiftOneEnd(ALogActor* L,float H){if(!L||H<=0||H>MaxLiftHeight)return false;return L->SetActorLocation(L->GetActorLocation()+FVector(0,0,H*0.5f),true);}
bool ULogJackingSystem::PlaceSupportUnder(ALogActor* L){if(!L||!GetWorld())return false;FHitResult H;const FVector S=L->GetActorLocation();const FVector E=S-FVector(0,0,MaxLiftHeight+100.0f);FCollisionQueryParams P(SCENE_QUERY_STAT(LogJack),false,L);return GetWorld()->LineTraceSingleByChannel(H,S,E,ECC_WorldStatic,P);}
bool ULogJackingSystem::IsFullySupported(ALogActor* L)const{return L&&L->IsLogGrounded();}
