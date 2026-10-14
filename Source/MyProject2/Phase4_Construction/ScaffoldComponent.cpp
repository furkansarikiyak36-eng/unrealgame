#include "Phase4_Construction/ScaffoldComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
UScaffoldComponent::UScaffoldComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UScaffoldComponent::BuildScaffold(const FVector& B,float H,float W){if(H<=0||W<=0||!GetWorld())return false;const int32 Levels=FMath::Clamp(FMath::CeilToInt(H/200.0f),1,64);for(int32 L=0;L<Levels;++L){const float Z=B.Z+L*200.0f;for(int32 X:{-1,1})for(int32 Y:{-1,1}){FVector P(B.X+X*W*0.5f,B.Y+Y*W*0.5f,Z);if(ALogActor* A=GetWorld()->SpawnActor<ALogActor>(LogClass.Get()?LogClass.Get():ALogActor::StaticClass(),P,FRotator::ZeroRotator)){if(DefaultDefinition)A->InitializeFromDefinition(DefaultDefinition);A->SetLogState(ELogState::Installed);ScaffoldParts.Add(A);}}}return !ScaffoldParts.IsEmpty();}
bool UScaffoldComponent::DemolishScaffold(){if(ScaffoldParts.IsEmpty())return false;for(ALogActor* A:ScaffoldParts)if(A){A->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);A->SetLogState(ELogState::Loose);}ScaffoldParts.Empty();return true;}
