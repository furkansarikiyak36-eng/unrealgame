#include "StructureDamageResolver.h"
#include "Phase1_Physical/LogActor.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "Phase5_Structural/DamageComponent.h"
bool UStructureDamageResolver::IsAttackValid(AActor*A,AActor*T,float R)const{return IsValid(A)&&IsValid(T)&&R>=0.f&&FVector::Distance(A->GetActorLocation(),T->GetActorLocation())<=R;}
float UStructureDamageResolver::ComputeDamage(float D,float R,float M)const{return FMath::Max(0.f,(D-FMath::Max(0.f,R))*FMath::Max(0.f,M));}
bool UStructureDamageResolver::ApplyDamageToBlock(AActor*B,float D){if(ALogActor*L=Cast<ALogActor>(B)){if(UDamageComponent*C=L->FindComponentByClass<UDamageComponent>()){C->ApplyDamage(D);return true;}L->SetHealth(L->GetHealth()-FMath::Max(0.f,D));return true;}if(AStoneActor*S=Cast<AStoneActor>(B)){S->ApplyDamage(D);return true;}if(AStickActor*S=Cast<AStickActor>(B)){S->ApplyDamage(D);return true;}return false;}
void UStructureDamageResolver::UpdateWorldAfterBlockDestroyed(UWorld*W,AActor*B){/* Integration hook: rebuild nearby path/navigation data after the actor is destroyed. */}
