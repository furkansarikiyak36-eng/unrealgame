#include "Phase2_Transport/LogPlacementComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Engine/World.h"
ULogPlacementComponent::ULogPlacementComponent(){PrimaryComponentTick.bCanEverTick=true;}
void ULogPlacementComponent::EnterAimMode(){bAiming=true;} void ULogPlacementComponent::ExitAimMode(){bAiming=false;} void ULogPlacementComponent::CycleSnap(){}
void ULogPlacementComponent::RotateLog(float D){TargetTransform.SetRotation((FQuat(FVector::UpVector,FMath::DegreesToRadians(D*15.0f))*TargetTransform.GetRotation()).GetNormalized());}
bool ULogPlacementComponent::ConfirmPlacement(){if(!PreviewLog)return false;PreviewLog->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);PreviewLog->SetActorTransform(TargetTransform);const bool bOk=PreviewLog->SetLogState(ELogState::Installed);if(bOk)PreviewLog=nullptr;return bOk;}
void ULogPlacementComponent::TickComponent(float D,ELevelTick T,FActorComponentTickFunction* F){Super::TickComponent(D,T,F);if(!bAiming||!GetOwner()||!GetWorld())return;const FVector S=GetOwner()->GetActorLocation();const FVector E=S+GetOwner()->GetActorForwardVector()*ReachDistance;FHitResult H;FCollisionQueryParams P(SCENE_QUERY_STAT(LogPlacement),false,GetOwner());if(GetWorld()->LineTraceSingleByChannel(H,S,E,ECC_Visibility,P)){TargetTransform.SetLocation(H.ImpactPoint);}}
