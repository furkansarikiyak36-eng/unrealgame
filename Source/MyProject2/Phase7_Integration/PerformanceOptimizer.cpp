#include "Phase7_Integration/PerformanceOptimizer.h"
#include "Phase1_Physical/LogActor.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"
void UPerformanceOptimizer::Initialize(FSubsystemCollectionBase& C){Super::Initialize(C);}
void UPerformanceOptimizer::Deinitialize(){Super::Deinitialize();}
void UPerformanceOptimizer::OptimizeAll(){UWorld* W=GetWorld();if(!W)return;int32 Active=0;for(TActorIterator<ALogActor> It(W);It;++It){ALogActor* L=*It;if(!L)continue;UPrimitiveComponent* P=Cast<UPrimitiveComponent>(L->GetRootComponent());if(!P)continue;if(L->GetLogState()==ELogState::Loose&&P->IsSimulatingPhysics()){if(Active<MaxActivePhysicsLogs){P->WakeRigidBody();++Active;}else P->PutRigidBodyToSleep();}}}
