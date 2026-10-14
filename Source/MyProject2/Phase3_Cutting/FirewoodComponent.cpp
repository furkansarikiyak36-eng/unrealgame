#include "Phase3_Cutting/FirewoodComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
UFirewoodComponent::UFirewoodComponent(){PrimaryComponentTick.bCanEverTick=false;}
TArray<ALogActor*> UFirewoodComponent::SplitIntoFirewood(ALogActor* L,int32 Pieces){TArray<ALogActor*> Out;if(!L||!L->GetWorld()||Pieces<1||Pieces>32||!L->GetDefinition())return Out;const FVector O=L->GetActorLocation();ULogDefinition* D=L->GetDefinition();for(int32 I=0;I<Pieces;++I){FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;if(ALogActor* N=L->GetWorld()->SpawnActor<ALogActor>(ALogActor::StaticClass(),O+FVector(I*25.0f,0,0),L->GetActorRotation(),P)){N->InitializeFromDefinition(D);N->ApplyCut(EWoodShape::Firewood,FMath::Max(5.0f,L->GetCurrentLength()/Pieces));Out.Add(N);}}if(Out.Num()>0)L->Destroy();return Out;}
