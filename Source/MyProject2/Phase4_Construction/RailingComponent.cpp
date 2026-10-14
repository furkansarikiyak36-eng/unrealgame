#include "Phase4_Construction/RailingComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
URailingComponent::URailingComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool URailingComponent::BuildRailing(const FVector& S,const FVector& E,float H){const float Dist=FVector::Dist(S,E);if(Dist<=0||H<=0||!GetWorld())return false;const int32 N=FMath::Clamp(FMath::CeilToInt(Dist/65.0f),1,256);for(int32 I=0;I<=N;++I){const FVector P=FMath::Lerp(S,E,(float)I/N)+FVector(0,0,H);if(ALogActor* A=GetWorld()->SpawnActor<ALogActor>(LogClass.Get()?LogClass.Get():ALogActor::StaticClass(),P,FRotator::ZeroRotator)){if(DefaultDefinition)A->InitializeFromDefinition(DefaultDefinition);A->SetLogState(ELogState::Installed);}}return true;}
