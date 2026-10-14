#include "StoneBeamComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneConnectionGraph.h"
#include "Engine/World.h"
UStoneBeamComponent::UStoneBeamComponent() { PrimaryComponentTick.bCanEverTick = false; }
AStoneActor* UStoneBeamComponent::BuildBeam(AStoneActor* LeftPillar, AStoneActor* RightPillar)
{
    if (!LeftPillar || !RightPillar || LeftPillar == RightPillar || !StoneClass || !GetWorld()) return nullptr;
    const FVector LeftTop = LeftPillar->GetActorLocation() + FVector(0, 0, 40.0f);
    const FVector RightTop = RightPillar->GetActorLocation() + FVector(0, 0, 40.0f);
    const float Span = FVector::Dist(LeftTop, RightTop);
    if (Span > MaxBeamSpan || Span <= KINDA_SMALL_NUMBER) return nullptr;
    const FVector Center = (LeftTop + RightTop) * 0.5f;
    const FVector Dir = (RightTop - LeftTop).GetSafeNormal();
    AStoneActor* Beam = GetWorld()->SpawnActor<AStoneActor>(StoneClass, Center, Dir.Rotation());
    if (!Beam) return nullptr;
    Beam->SetStructuralState_Implementation(EStructuralNodeState::Installed);
    if (UStoneConnectionGraph* G = Beam->FindComponentByClass<UStoneConnectionGraph>())
    {
        G->ConnectTo(LeftPillar, TEXT("Left"), TEXT("Top"), EConnectionType::Stacked, true);
        G->ConnectTo(RightPillar, TEXT("Right"), TEXT("Top"), EConnectionType::Stacked, true);
    }
    return Beam;
}
bool UStoneBeamComponent::IsBeamSupported(AStoneActor* Beam) const
{
    if (!Beam) return false;
    if (UStoneConnectionGraph* G = Beam->FindComponentByClass<UStoneConnectionGraph>()) return G->GetConnectedNodes().Num() >= 2;
    return false;
}
