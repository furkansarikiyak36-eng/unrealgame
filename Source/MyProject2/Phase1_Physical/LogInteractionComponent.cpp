#include "Phase1_Physical/LogInteractionComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Phase1_Physical/LogPhysicsComponent.h"
ULogInteractionComponent::ULogInteractionComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool ULogInteractionComponent::TryInteract(ALogActor* Target) const{return Target&&GetOwner()&&FVector::DistSquared(GetOwner()->GetActorLocation(),Target->GetActorLocation())<=FMath::Square(InteractRange);}
void ULogInteractionComponent::PushLog(ALogActor* T,float S){if(!TryInteract(T))return;const FVector D=(T->GetActorLocation()-GetOwner()->GetActorLocation()).GetSafeNormal();if(auto* P=T->FindComponentByClass<ULogPhysicsComponent>())P->ApplyPush(D,FMath::Max(0.0f,S)*PushStrength);}
void ULogInteractionComponent::KickLog(ALogActor* T,float S){PushLog(T,S*2.0f);}
