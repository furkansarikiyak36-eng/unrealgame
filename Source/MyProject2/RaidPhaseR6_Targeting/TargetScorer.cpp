#include "TargetScorer.h"
#include "GameFramework/Actor.h"
float UTargetScorer::ScoreTarget(AActor*S,AActor*T)const{if(!S||!T||!IsValid(T))return -TNumericLimits<float>::Max();const float D=FVector::Distance(S->GetActorLocation(),T->GetActorLocation());FHitResult H;FCollisionQueryParams P;P.AddIgnoredActor(S);P.AddIgnoredActor(T);const bool Visible=!S->GetWorld()->LineTraceSingleByChannel(H,S->GetActorLocation(),T->GetActorLocation(),ECC_Visibility,P);return WeightDistance/(1.f+D*.001f)+WeightVisibility*(Visible?1.f:.3f)+WeightThreat+WeightAccessibility;}
AActor* UTargetScorer::PickBestTarget(AActor*S,const TArray<AActor*>&Cs)const{AActor*B=nullptr;float V=-TNumericLimits<float>::Max();for(AActor*C:Cs){const float N=ScoreTarget(S,C);if(N>V){V=N;B=C;}}return B;}
