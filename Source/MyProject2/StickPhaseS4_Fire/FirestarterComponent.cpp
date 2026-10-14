#include "FirestarterComponent.h"
UFirestarterComponent::UFirestarterComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UFirestarterComponent::StrikeStone(const FVector&){++AttemptCount;if(AttemptCount>=FMath::Max(1,AttemptsRequired)&&FMath::FRand()<=IgnitionChance){AttemptCount=0;return true;}return false;}
bool UFirestarterComponent::RubSticks(const FVector&L){AttemptsRequired=5;return StrikeStone(L);} bool UFirestarterComponent::UseFlint(const FVector&L){IgnitionChance=.95f;AttemptsRequired=1;return StrikeStone(L);}
