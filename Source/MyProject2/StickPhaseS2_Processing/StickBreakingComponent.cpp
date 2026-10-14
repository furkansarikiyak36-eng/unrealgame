#include "StickBreakingComponent.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "StickCore/StickDefinition.h"
#include "Engine/World.h"
UStickBreakingComponent::UStickBreakingComponent(){PrimaryComponentTick.bCanEverTick=true;}
bool UStickBreakingComponent::BeginBreak(AStickActor*S){if(!S||bIsBreaking)return false;BreakingStick=S;bIsBreaking=true;BreakProgress=0;return true;}
bool UStickBreakingComponent::CompleteBreak(){if(!BreakingStick)return false;AStickActor* S=BreakingStick;const FVector P=S->GetActorLocation();for(int32 i=0;i<FMath::Max(1,PiecesPerBreak);++i){if(AStickActor*N=GetWorld()->SpawnActor<AStickActor>(S->GetClass(),P+FVector(i*5,0,0),S->GetActorRotation())){if(S->GetDefinition())N->InitializeFromDefinition(S->GetDefinition());N->ApplyShape(EStickShape::Broken);}}S->Destroy();BreakingStick=nullptr;bIsBreaking=false;return true;}
bool UStickBreakingComponent::CancelBreak(){if(!bIsBreaking)return false;bIsBreaking=false;BreakingStick=nullptr;BreakProgress=0;return true;}
void UStickBreakingComponent::TickComponent(float D,ELevelTick T,FActorComponentTickFunction* F){Super::TickComponent(D,T,F);if(!bIsBreaking||!BreakingStick)return;BreakProgress+=D;if(BreakProgress>=FMath::Max(BreakDuration,0.01f))CompleteBreak();}
