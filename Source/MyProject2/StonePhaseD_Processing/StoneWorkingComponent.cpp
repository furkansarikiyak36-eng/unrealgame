#include "StoneWorkingComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "Engine/World.h"
UStoneWorkingComponent::UStoneWorkingComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UStoneWorkingComponent::CrushBoulder(AStoneActor* Boulder, int32 Pieces)
{
    if (!Boulder || Pieces <= 0 || !StoneClass || !GetWorld()) return false;
    const FVector Origin = Boulder->GetActorLocation();
    int32 Spawned = 0;
    for (int32 i = 0; i < Pieces; ++i)
    {
        const FVector Offset(FMath::RandRange(-50.f, 50.f), FMath::RandRange(-50.f, 50.f), FMath::RandRange(0.f, 20.f));
        if (AStoneActor* Piece = GetWorld()->SpawnActor<AStoneActor>(StoneClass, Origin + Offset, FRotator::ZeroRotator)) ++Spawned;
    }
    if (Spawned == 0) return false;
    Boulder->Destroy(); return true;
}
bool UStoneWorkingComponent::ProduceBuildingStone(AStoneActor* Boulder) { return CrushBoulder(Boulder, 4); }
bool UStoneWorkingComponent::ProduceFlatStone(AStoneActor* Stone) { if (!Stone) return false; Stone->ApplyShape(EStoneShape::Flat, FVector(40, 40, 10)); return true; }
bool UStoneWorkingComponent::CarveSurface(AStoneActor* Stone) { if (!Stone) return false; Stone->ApplyShape(EStoneShape::Carved, FVector(35, 25, 20)); return true; }
bool UStoneWorkingComponent::CutCorner(AStoneActor* Stone)
{
    if (!Stone || !Stone->GetDefinition()) return false;
    const FVector D = Stone->GetDefinition()->Dimensions;
    Stone->ApplyShape(EStoneShape::Carved, FVector(D.X * 0.9f, D.Y * 0.9f, D.Z)); return true;
}
bool UStoneWorkingComponent::HollowGroove(AStoneActor* Stone) { return Stone != nullptr; }
bool UStoneWorkingComponent::EngraveJoint(AStoneActor* Stone) { return Stone != nullptr; }
bool UStoneWorkingComponent::SmoothSurface(AStoneActor* Stone) { return Stone != nullptr; }
bool UStoneWorkingComponent::ProduceGravel(AStoneActor* Stone, int32 Pieces) { return CrushBoulder(Stone, Pieces); }
