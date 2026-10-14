#include "MortarComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneConnectionGraph.h"
UMortarComponent::UMortarComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UMortarComponent::PrepareMortar(int32 SandAmount, int32 LimeAmount, int32 WaterAmount)
{
    if (SandAmount <= 0 || LimeAmount <= 0 || WaterAmount <= 0) return false;
    const int32 MinAmount = FMath::Min3(SandAmount, LimeAmount, WaterAmount);
    MortarAmount += static_cast<float>(MinAmount); return true;
}
bool UMortarComponent::ApplyMortar(AStoneActor* StoneA, AStoneActor* StoneB)
{
    if (!StoneA || !StoneB || MortarAmount < 1.0f) return false;
    UStoneConnectionGraph* Graph = StoneA->FindComponentByClass<UStoneConnectionGraph>();
    if (!Graph || !Graph->MortarConnection(StoneB, MortarStrengthMultiplier)) return false;
    MortarAmount -= 1.0f; return true;
}
bool UMortarComponent::FillJoint(AStoneActor* Stone)
{
    if (!Stone || MortarAmount < 0.5f) return false;
    MortarAmount -= 0.5f; return true;
}
