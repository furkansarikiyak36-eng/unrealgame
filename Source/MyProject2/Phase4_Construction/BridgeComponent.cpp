#include "Phase4_Construction/BridgeComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
#include "WorldCore/RegionLogisticsSubsystem.h"
#include "Engine/World.h"
UBridgeComponent::UBridgeComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UBridgeComponent::BuildBridge(const FVector& S,const FVector& E,bool bNeedsSupport)
{
    const float Span=FVector::Dist(S,E);
    if(Span<=0||(!bNeedsSupport&&Span>MaxSpanWithoutSupport)||!GetWorld())return false;
    const int32 N=FMath::CeilToInt(Span/65.0f);int32 Made=0;const FVector D=(E-S).GetSafeNormal();
    for(int32 I=0;I<N;++I){const FVector L=S+D*FMath::Min(I*65.0f+32.5f,Span);if(ALogActor* A=GetWorld()->SpawnActor<ALogActor>(LogClass.Get()?LogClass.Get():ALogActor::StaticClass(),L,D.Rotation())){if(DefaultDefinition)A->InitializeFromDefinition(DefaultDefinition);A->InfrastructureLinkKey=LogisticsLinkKey;A->SetLogState(ELogState::Installed);++Made;}}
    const bool bBuilt=Made==N;
    if(bBuilt && !LogisticsLinkKey.IsNone())
    {
        const FName StartKey=LogisticsStartNodeKey.IsNone()?FName(*(LogisticsLinkKey.ToString()+TEXT("_START"))):LogisticsStartNodeKey;
        const FName EndKey=LogisticsEndNodeKey.IsNone()?FName(*(LogisticsLinkKey.ToString()+TEXT("_END"))):LogisticsEndNodeKey;
        if(URegionLogisticsSubsystem* Logistics=GetWorld()->GetSubsystem<URegionLogisticsSubsystem>())
        {
            if(!Logistics->RegisterInfrastructureSegment(LogisticsLinkKey,StartKey,S,EndKey,E,ELogisticsLinkType::Bridge,LogisticsCapacityKg,1.0f,LogisticsRisk01,true,true))
            {
                UE_LOG(LogTemp,Warning,TEXT("Bridge built, but optional route registration failed for key %s."),*LogisticsLinkKey.ToString());
            }
        }
    }
    return bBuilt;
}
void UBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if(!LogisticsLinkKey.IsNone())
    {
        if(UWorld* World=GetWorld()) if(URegionLogisticsSubsystem* Logistics=World->GetSubsystem<URegionLogisticsSubsystem>()) Logistics->UnregisterInfrastructureSegment(LogisticsLinkKey);
    }
    Super::EndPlay(EndPlayReason);
}
