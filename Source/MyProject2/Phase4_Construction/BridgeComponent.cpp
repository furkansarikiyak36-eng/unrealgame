#include "Phase4_Construction/BridgeComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
UBridgeComponent::UBridgeComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UBridgeComponent::BuildBridge(const FVector& S,const FVector& E,bool bNeedsSupport){const float Span=FVector::Dist(S,E);if(Span<=0||(!bNeedsSupport&&Span>MaxSpanWithoutSupport)||!GetWorld())return false;const int32 N=FMath::CeilToInt(Span/65.0f);int32 Made=0;const FVector D=(E-S).GetSafeNormal();for(int32 I=0;I<N;++I){const FVector L=S+D*FMath::Min(I*65.0f+32.5f,Span);if(ALogActor* A=GetWorld()->SpawnActor<ALogActor>(LogClass.Get()?LogClass.Get():ALogActor::StaticClass(),L,D.Rotation())){if(DefaultDefinition)A->InitializeFromDefinition(DefaultDefinition);A->SetLogState(ELogState::Installed);++Made;}}return Made==N;}
