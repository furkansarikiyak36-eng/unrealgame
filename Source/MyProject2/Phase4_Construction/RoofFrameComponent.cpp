#include "Phase4_Construction/RoofFrameComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
URoofFrameComponent::URoofFrameComponent(){PrimaryComponentTick.bCanEverTick=false;}
static ALogActor* RoofSpawn(UWorld* W,TSubclassOf<ALogActor> C,ULogDefinition* D,const FVector& L,const FRotator& R){if(!W)return nullptr;ALogActor* A=W->SpawnActor<ALogActor>(C.Get()?C.Get():ALogActor::StaticClass(),L,R);if(A){if(D)A->InitializeFromDefinition(D);A->SetLogState(ELogState::Installed);}return A;}
bool URoofFrameComponent::BuildRidgeBeam(const FVector& S,const FVector& E){return !S.Equals(E)&&RoofSpawn(GetWorld(),LogClass,DefaultDefinition,(S+E)*0.5f,(E-S).Rotation())!=nullptr;}
bool URoofFrameComponent::BuildPurlin(const FVector& S,const FVector& E){return BuildRidgeBeam(S,E);} bool URoofFrameComponent::BuildRafter(const FVector& S,const FVector& E){return BuildRidgeBeam(S,E);}
bool URoofFrameComponent::CoverWithPlanks(const TArray<FVector>& Ps){if(Ps.IsEmpty())return false;bool bAll=true;for(const FVector& P:Ps)bAll=RoofSpawn(GetWorld(),LogClass,DefaultDefinition,P,FRotator::ZeroRotator)!=nullptr&&bAll;return bAll;}
