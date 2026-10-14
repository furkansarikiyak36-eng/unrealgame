#include "ArchComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneConnectionGraph.h"
#include "Engine/World.h"
UArchComponent::UArchComponent() { PrimaryComponentTick.bCanEverTick = false; }
TArray<AStoneActor*> UArchComponent::BuildArch(const FVector& LeftBase, const FVector& RightBase, float Height, int32 VoussoirCount)
{
    TArray<AStoneActor*> Result;
    if (VoussoirCount < 3 || !StoneClass || Height <= 0.0f || !GetWorld()) return Result;
    const FVector Center = (LeftBase + RightBase) * 0.5f;
    const float Span = FVector::Dist(LeftBase, RightBase);
    const float HalfSpan = Span * 0.5f;
    // Elliptic approximation for gameplay placement, not a structural stress solver.
    AStoneActor* Previous = nullptr;
    for (int32 i = 0; i < VoussoirCount; ++i)
    {
        const float T = static_cast<float>(i) / static_cast<float>(VoussoirCount - 1);
        const float Angle = PI * T;
        const FVector Pos = Center + FVector(FMath::Cos(Angle) * HalfSpan, 0.0f, FMath::Sin(Angle) * Height);
        const FVector Crown = Center + FVector(0, 0, Height);
        const FRotator Rot = (Crown - Pos).GetSafeNormal().Rotation();
        AStoneActor* Voussoir = GetWorld()->SpawnActor<AStoneActor>(StoneClass, Pos, Rot);
        if (!Voussoir) continue;
        Voussoir->SetStructuralState_Implementation(EStructuralNodeState::Installed);
        Result.Add(Voussoir);
        if (Previous)
            if (UStoneConnectionGraph* G = Previous->FindComponentByClass<UStoneConnectionGraph>())
                G->ConnectArchVoussoir(Voussoir, TEXT("ArchRight"), TEXT("ArchLeft"));
        Previous = Voussoir;
    }
    return Result;
}
bool UArchComponent::IsArchStable(const TArray<AStoneActor*>& Arch) const
{
    if (Arch.Num() < 3) return false;
    const int32 KeystoneIdx = Arch.Num() / 2;
    AStoneActor* Keystone = Arch[KeystoneIdx];
    if (!Keystone) return false;
    if (UStoneConnectionGraph* G = Keystone->FindComponentByClass<UStoneConnectionGraph>()) return G->GetConnectedNodes().Num() >= 2;
    return false;
}
