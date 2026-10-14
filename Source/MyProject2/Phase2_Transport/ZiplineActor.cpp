#include "Phase2_Transport/ZiplineActor.h"
#include "Phase1_Physical/LogActor.h"
#include "Components/SplineComponent.h"
AZiplineActor::AZiplineActor(){PrimaryActorTick.bCanEverTick=true;Cable=CreateDefaultSubobject<USplineComponent>(TEXT("Cable"));RootComponent=Cable;}
bool AZiplineActor::AttachLog(ALogActor* L){if(!L||AttachedLog||Cable->GetNumberOfSplinePoints()<2||!L->SetLogState(ELogState::Installed))return false;AttachedLog=L;DistanceOnSpline=0.0f;L->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);return true;}
void AZiplineActor::Tick(float D){Super::Tick(D);if(!AttachedLog)return;DistanceOnSpline+=SlideSpeed*D;const float Len=Cable->GetSplineLength();if(DistanceOnSpline>=Len){ReleaseAtEnd();return;}AttachedLog->SetActorLocation(Cable->GetLocationAtDistanceAlongSpline(DistanceOnSpline,ESplineCoordinateSpace::World));AttachedLog->SetActorRotation(Cable->GetRotationAtDistanceAlongSpline(DistanceOnSpline,ESplineCoordinateSpace::World));}
void AZiplineActor::ReleaseAtEnd(){if(!AttachedLog)return;ALogActor* L=AttachedLog;AttachedLog=nullptr;L->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);L->SetLogState(ELogState::Falling);}
