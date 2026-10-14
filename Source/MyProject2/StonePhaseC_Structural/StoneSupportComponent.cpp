#include "StoneSupportComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneConnectionGraph.h"
UStoneSupportComponent::UStoneSupportComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UStoneSupportComponent::CanRemoveColumn(AStoneActor* Column) const { return GetOrphansIfRemoved(Column).Num() == 0; }
TArray<AStoneActor*> UStoneSupportComponent::GetOrphansIfRemoved(AStoneActor* Column) const
{
    TArray<AStoneActor*> Result;
    if (!Column) return Result;
    if (UStoneConnectionGraph* G = Column->FindComponentByClass<UStoneConnectionGraph>())
        for (AActor* N : G->SimulateWithout(Column)) if (AStoneActor* S = Cast<AStoneActor>(N)) Result.Add(S);
    return Result;
}
bool UStoneSupportComponent::AddBrace(AStoneActor* Wall, AStoneActor* Brace)
{
    if (!Wall || !Brace) return false;
    if (UStoneConnectionGraph* G = Wall->FindComponentByClass<UStoneConnectionGraph>())
        return G->ConnectTo(Brace, TEXT("Brace"), TEXT("Wall"), EConnectionType::ButtJoint, true);
    return false;
}
bool UStoneSupportComponent::IsLaterallyStable(AStoneActor* Wall) const
{
    if (!Wall) return false;
    if (UStoneConnectionGraph* G = Wall->FindComponentByClass<UStoneConnectionGraph>())
    {
        int32 Braces = 0;
        for (const FStructuralConnection& C : G->Connections) if (C.ConnectionType == EConnectionType::ButtJoint) ++Braces;
        return Braces >= 2;
    }
    return false;
}
