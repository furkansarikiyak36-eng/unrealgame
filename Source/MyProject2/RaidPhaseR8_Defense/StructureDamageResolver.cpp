#include "StructureDamageResolver.h"
#include "Phase1_Physical/LogActor.h"
#include "WorldCore/RegionLogisticsSubsystem.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "Phase5_Structural/DamageComponent.h"
#include "NavigationSystem.h"
#include "Engine/World.h"

bool UStructureDamageResolver::IsAttackValid(AActor* Attacker,AActor* Target,float Range) const
{
    return IsValid(Attacker)&&IsValid(Target)&&FMath::IsFinite(Range)&&Range>=0.f
        && FVector::Dist(Attacker->GetActorLocation(),Target->GetActorLocation())<=Range;
}
float UStructureDamageResolver::ComputeDamage(float RawDamage,float Resistance,float Multiplier) const
{
    if(!FMath::IsFinite(RawDamage)||!FMath::IsFinite(Resistance)||!FMath::IsFinite(Multiplier)||RawDamage<=0.f||Multiplier<=0.f)
        return 0.f;
    return FMath::Max(0.f,(RawDamage-FMath::Max(0.f,Resistance))*Multiplier);
}
bool UStructureDamageResolver::ApplyDamageToBlock(AActor* Block,float Damage)
{
    if(!IsValid(Block)||!FMath::IsFinite(Damage)||Damage<=0.f||Block->IsActorBeingDestroyed())return false;
    const FBox PreviousBounds=Block->GetComponentsBoundingBox(true);
    bool bApplied=false;
    if(ALogActor* Log=Cast<ALogActor>(Block))
    {
        if(UDamageComponent* Component=Log->FindComponentByClass<UDamageComponent>())Component->ApplyDamage(Damage);
        else { Log->SetHealth(Log->GetHealth()-Damage); if(Log->GetHealth()<=0.f)Log->SetLogState(ELogState::Destroyed); }
        bApplied=true;
    }
    else if(AStoneActor* Stone=Cast<AStoneActor>(Block))
    {
        Stone->ApplyDamage(Damage);bApplied=true;
    }
    else if(AStickActor* Stick=Cast<AStickActor>(Block))
    {
        Stick->ApplyDamage(Damage);bApplied=true;
    }
    if(bApplied&&Block->IsActorBeingDestroyed())
    {
        if(UWorld* World=Block->GetWorld())
        {
            if(UNavigationSystemV1* Navigation=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
            {
                if(PreviousBounds.IsValid)Navigation->AddDirtyArea(PreviousBounds.ExpandBy(100.f),ENavigationDirtyFlag::All);
            }
            if(const ALogActor* Log=Cast<ALogActor>(Block);Log&&!Log->InfrastructureLinkKey.IsNone())
            {
                if(URegionLogisticsSubsystem* Logistics=World->GetSubsystem<URegionLogisticsSubsystem>())
                {
                    // A destroyed piece means this bridge segment cannot be trusted for hauling until repaired/re-registered.
                    Logistics->SetInfrastructureStatus(Log->InfrastructureLinkKey,false,0.0f,1.0f);
                }
            }
        }
    }
    return bApplied;
}
void UStructureDamageResolver::UpdateWorldAfterBlockDestroyed(UWorld* World,AActor* Block)
{
    if(!World||!IsValid(Block))return;
    const FBox Bounds=Block->GetComponentsBoundingBox(true);
    if(!Bounds.IsValid)return;
    if(UNavigationSystemV1* Navigation=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        Navigation->AddDirtyArea(Bounds.ExpandBy(100.f),ENavigationDirtyFlag::All);
    }
}
