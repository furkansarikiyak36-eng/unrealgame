#include "Phase4_Construction/ConstructionComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
UConstructionComponent::UConstructionComponent(){PrimaryComponentTick.bCanEverTick=false;}
static ALogActor* SpawnBuildLog(UWorld* W,TSubclassOf<ALogActor> C,ULogDefinition* Def,const FVector& L,const FRotator& R){if(!W)return nullptr;ALogActor* A=W->SpawnActor<ALogActor>(C.Get()?C.Get():ALogActor::StaticClass(),L,R);if(A){if(Def)A->InitializeFromDefinition(Def);A->SetLogState(ELogState::Installed);}return A;}
ALogActor* UConstructionComponent::PlacePillar(const FVector& L){return SpawnBuildLog(GetWorld(),LogClass,DefaultDefinition,L,FRotator::ZeroRotator);}
ALogActor* UConstructionComponent::PlaceBeam(const FVector& S,const FVector& E){if(S.Equals(E))return nullptr;return SpawnBuildLog(GetWorld(),LogClass,DefaultDefinition,(S+E)*0.5f,(E-S).Rotation());}
ALogActor* UConstructionComponent::PlaceBrace(const FVector& S,const FVector& E){return PlaceBeam(S,E);}
ALogActor* UConstructionComponent::PlaceFloor(const FVector& L,const FVector& N){return SpawnBuildLog(GetWorld(),LogClass,DefaultDefinition,L,N.Rotation());}
ALogActor* UConstructionComponent::PlaceWall(const FVector& L,const FVector& D,bool V){const FRotator R=V?FRotator(0,D.Rotation().Yaw,0):D.Rotation();return SpawnBuildLog(GetWorld(),LogClass,DefaultDefinition,L,R);}
