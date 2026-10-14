#include "Phase5_Structural/FireSpreadComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
#include "Engine/World.h"
#include "EngineUtils.h"
UFireSpreadComponent::UFireSpreadComponent(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=0.5f;}
void UFireSpreadComponent::Ignite(float H){if(bBurning)return;Heat=FMath::Max(0.0f,H);bBurning=true;if(ALogActor* L=Cast<ALogActor>(GetOwner()))L->SetLogState(ELogState::Burning);}
void UFireSpreadComponent::Extinguish(){Heat=0;bBurning=false;if(ALogActor* L=Cast<ALogActor>(GetOwner()))if(L->GetLogState()==ELogState::Burning)L->SetLogState(ELogState::Loose);}
void UFireSpreadComponent::TickComponent(float D,ELevelTick T,FActorComponentTickFunction* F){Super::TickComponent(D,T,F);if(!bBurning||!GetWorld())return;Heat+=HeatGainPerSecond*D;SpreadCooldown-=D;
    if(ALogActor* L=Cast<ALogActor>(GetOwner())){BurnRatePerSecond=FMath::Max(0.0f,BurnRatePerSecond);if(L->GetDefinition())L->ApplyCut(L->GetCurrentShape(),FMath::Max(1.0f,L->GetCurrentLength()-BurnRatePerSecond*D));}
    if(SpreadCooldown<=0){SpreadCooldown=1.0f;for(TActorIterator<ALogActor> It(GetWorld());It;++It){ALogActor* Other=*It;if(!Other||Other==GetOwner()||FVector::DistSquared(GetOwner()->GetActorLocation(),Other->GetActorLocation())>FMath::Square(SpreadRadius))continue;if(auto* Fire=Other->FindComponentByClass<UFireSpreadComponent>())if(!Fire->IsBurning()&&Heat>=SpreadThreshold&&Other->GetDefinition()&&Other->GetDefinition()->Flammability>0)Fire->Ignite(Heat*0.5f);}}
    if(ALogActor* L=Cast<ALogActor>(GetOwner()))if(L->GetCurrentLength()<=1.01f){bBurning=false;L->SetLogState(ELogState::Destroyed);}
}
