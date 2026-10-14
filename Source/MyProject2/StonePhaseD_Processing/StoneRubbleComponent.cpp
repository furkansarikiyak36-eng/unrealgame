#include "StoneRubbleComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
UStoneRubbleComponent::UStoneRubbleComponent() { PrimaryComponentTick.bCanEverTick = false; }
int32 UStoneRubbleComponent::RecoverMaterial(AStoneActor* Rubble)
{
    if (!Rubble || !Rubble->GetDefinition()) return 0;
    const int32 Amount = FMath::RoundToInt(Rubble->GetDefinition()->Mass * FMath::Max(0.0f, RecoveryRate));
    Rubble->Destroy(); return Amount;
}
bool UStoneRubbleComponent::ReuseAsGravel(AStoneActor* Rubble)
{
    if (!Rubble) return false;
    Rubble->ApplyShape(EStoneShape::Gravel, FVector(10, 10, 5)); return true;
}
