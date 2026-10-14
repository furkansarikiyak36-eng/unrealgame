#include "Phase2_Transport/LogCarryComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
ULogCarryComponent::ULogCarryComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool ULogCarryComponent::PickUp(ALogActor* L,bool bShoulder){if(!L||CarriedLog||!GetOwner())return false;ULogDefinition* D=L->GetDefinition();if(D&&D->Mass>MaxCarryMass)return false;ACharacter* C=Cast<ACharacter>(GetOwner());if(!C||!C->GetMesh())return false;const FName S=bShoulder?ShoulderSocket:HandSocket;if(!C->GetMesh()->DoesSocketExist(S))return false;if(!L->SetLogState(ELogState::Carried))return false;L->AttachToComponent(C->GetMesh(),FAttachmentTransformRules::SnapToTargetNotIncludingScale,S);CarriedLog=L;return true;}
bool ULogCarryComponent::Drop(){if(!CarriedLog)return false;ALogActor* L=CarriedLog;CarriedLog=nullptr;L->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);return L->SetLogState(ELogState::Loose);}
bool ULogCarryComponent::Throw(float Charge){if(!CarriedLog||!GetOwner())return false;ALogActor* L=CarriedLog;CarriedLog=nullptr;L->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);L->SetLogState(ELogState::Falling);if(UPrimitiveComponent* P=Cast<UPrimitiveComponent>(L->GetRootComponent()))P->AddImpulse(GetOwner()->GetActorForwardVector()*FMath::Max(0.0f,Charge)*800.0f,NAME_None,true);return true;}
