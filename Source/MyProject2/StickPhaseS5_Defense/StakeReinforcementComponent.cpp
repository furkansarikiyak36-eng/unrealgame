#include "StakeReinforcementComponent.h"

#include "StakeActor.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "Core/StructuralMaterialTypes.h"

UStakeReinforcementComponent::UStakeReinforcementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UStakeReinforcementComponent::ReinforceWithStones(AStakeActor* Stake, const TArray<AStoneActor*>& Stones)
{
	if (!IsValid(Stake) || Stones.Num() != StonesRequired || Stake->bReinforced) return false;
	TSet<const AStoneActor*> UniqueStones;
	for (AStoneActor* Stone : Stones)
	{
		if (!IsValid(Stone) || UniqueStones.Contains(Stone)) return false;
		const EStructuralNodeState State = Stone->GetStructuralState_Implementation();
		if (State != EStructuralNodeState::Loose && State != EStructuralNodeState::Carried) return false;
		UniqueStones.Add(Stone);
	}
	if (!Stake->ApplyStoneReinforcement(Stones.Num(), StrengthBonusPerStone)) return false;
	for (AStoneActor* Stone : Stones) Stone->Destroy();
	return true;
}

bool UStakeReinforcementComponent::ReinforceWithStones(AStakeActor* Stake, int32 StoneCount)
{
	if (!bAllowCountOnlyReinforcement || !IsValid(Stake) || StoneCount != StonesRequired || Stake->bReinforced)
	{
		return false;
	}
	return Stake->ApplyStoneReinforcement(StoneCount, StrengthBonusPerStone);
}
