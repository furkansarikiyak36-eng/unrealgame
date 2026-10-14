#include "Phase4_Construction/StairComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
UStairComponent::UStairComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UStairComponent::BuildStair(const FVector& B,const FVector& T,int32 Steps){if(Steps<=0||Steps>128||!GetWorld())return false;int32 Made=0;for(int32 I=0;I<Steps;++I){const float A=(float)I/(float)Steps;const FVector L=FMath::Lerp(B,T,A)+FVector(0,0,StepHeight*I);ALogActor* S=GetWorld()->SpawnActor<ALogActor>(LogClass.Get()?LogClass.Get():ALogActor::StaticClass(),L,FRotator::ZeroRotator);if(S){if(DefaultDefinition)S->InitializeFromDefinition(DefaultDefinition);S->SetLogState(ELogState::Installed);++Made;}}return Made==Steps;}
