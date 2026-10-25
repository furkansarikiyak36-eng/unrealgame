#include "Phase2_Transport/ZiplineActor.h"
#include "Phase1_Physical/LogActor.h"
#include "Components/SplineComponent.h"
#include "WorldCore/RegionLogisticsSubsystem.h"
#include "Engine/World.h"
AZiplineActor::AZiplineActor(){PrimaryActorTick.bCanEverTick=true;Cable=CreateDefaultSubobject<USplineComponent>(TEXT("Cable"));RootComponent=Cable;}
void AZiplineActor::BeginPlay(){Super::BeginPlay();RefreshLogisticsRoute();}
bool AZiplineActor::RefreshLogisticsRoute()
{
    if(LogisticsLinkKey.IsNone()||!Cable||Cable->GetNumberOfSplinePoints()<2||!GetWorld())return false;
    const FName StartKey=LogisticsStartNodeKey.IsNone()?FName(*(LogisticsLinkKey.ToString()+TEXT("_START"))):LogisticsStartNodeKey;
    const FName EndKey=LogisticsEndNodeKey.IsNone()?FName(*(LogisticsLinkKey.ToString()+TEXT("_END"))):LogisticsEndNodeKey;
    URegionLogisticsSubsystem* Logistics=GetWorld()->GetSubsystem<URegionLogisticsSubsystem>();
    return Logistics&&Logistics->RegisterInfrastructureSegment(LogisticsLinkKey,StartKey,Cable->GetLocationAtSplinePoint(0,ESplineCoordinateSpace::World),EndKey,Cable->GetLocationAtSplinePoint(Cable->GetNumberOfSplinePoints()-1,ESplineCoordinateSpace::World),ELogisticsLinkType::Ropeway,LogisticsCapacityKg,1.0f,LogisticsRisk01,true,true);
}
bool AZiplineActor::AttachLog(ALogActor* L){if(!L||AttachedLog||Cable->GetNumberOfSplinePoints()<2||!L->SetLogState(ELogState::Installed))return false;AttachedLog=L;DistanceOnSpline=0.0f;L->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);return true;}
void AZiplineActor::Tick(float D){Super::Tick(D);if(!AttachedLog)return;DistanceOnSpline+=SlideSpeed*D;const float Len=Cable->GetSplineLength();if(DistanceOnSpline>=Len){ReleaseAtEnd();return;}AttachedLog->SetActorLocation(Cable->GetLocationAtDistanceAlongSpline(DistanceOnSpline,ESplineCoordinateSpace::World));AttachedLog->SetActorRotation(Cable->GetRotationAtDistanceAlongSpline(DistanceOnSpline,ESplineCoordinateSpace::World));}
void AZiplineActor::EndPlay(const EEndPlayReason::Type EndPlayReason){if(!LogisticsLinkKey.IsNone())if(UWorld* World=GetWorld())if(URegionLogisticsSubsystem* Logistics=World->GetSubsystem<URegionLogisticsSubsystem>())Logistics->UnregisterInfrastructureSegment(LogisticsLinkKey);Super::EndPlay(EndPlayReason);}
void AZiplineActor::ReleaseAtEnd(){if(!AttachedLog)return;ALogActor* L=AttachedLog;AttachedLog=nullptr;L->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);L->SetLogState(ELogState::Falling);}
