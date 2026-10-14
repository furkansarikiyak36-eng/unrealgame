#include "ActionCostComparator.h"
float UActionCostComparator::ComputeRouteCost(const FVector&S,const FVector&G)const{const float D=FVector::Distance(S,G);return D*FMath::Max(0.f,MovementWeight)+D*.1f*FMath::Max(0.f,DelayWeight)+D*.05f*FMath::Max(0.f,RiskWeight);}
float UActionCostComparator::ComputeBreakCost(AActor*O)const{return O?100.f*FMath::Max(0.f,AttackWeight)+2.f*FMath::Max(0.f,TimeWeight):TNumericLimits<float>::Max();}
EAlternativeAction UActionCostComparator::ChooseCheaperAction(const FVector&S,const FVector&G,AActor*O)const{return ComputeRouteCost(S,G)<ComputeBreakCost(O)?EAlternativeAction::RecalculatePath:EAlternativeAction::AttackObstacle;}
