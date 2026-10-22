#include "PlaceableShoringPiece.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

APlaceableShoringPiece::APlaceableShoringPiece()
{
    PrimaryActorTick.bCanEverTick = false;
    PieceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PieceMesh"));
    SetRootComponent(PieceMesh);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        PieceMesh->SetStaticMesh(CubeMesh.Object);
    }
    PieceMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    PieceMesh->SetCollisionObjectType(ECC_WorldDynamic);
    PieceMesh->SetCollisionResponseToAllChannels(ECR_Block);
    PieceMesh->SetGenerateOverlapEvents(true);
    PieceMesh->SetCanEverAffectNavigation(true);
    Tags.AddUnique(TEXT("World.Object.Structure"));
}

void APlaceableShoringPiece::InitializeFromCandidate(const FShoringPlacementCandidate& Candidate)
{
    PieceGuid = Candidate.CandidateId.IsValid() ? Candidate.CandidateId : FGuid::NewGuid();
    PieceType = Candidate.PieceType;
    AnchorId = Candidate.AnchorId;
    DimensionsCm = Candidate.DimensionsCm;
    StructuralHealth = 100.0f;
    SetActorTransform(Candidate.Transform);
    SetActorScale3D(FVector(
        FMath::Max(0.01f, DimensionsCm.X / 100.0f),
        FMath::Max(0.01f, DimensionsCm.Y / 100.0f),
        FMath::Max(0.01f, DimensionsCm.Z / 100.0f)));
    bValidatedPlacement = Candidate.bPlanValid;
    if (Candidate.PieceType == EShoringPieceType::Upright
        || Candidate.PieceType == EShoringPieceType::WallBoard
        || Candidate.PieceType == EShoringPieceType::Strut)
    {
        Tags.AddUnique(TEXT("World.Shoring"));
    }
}

void APlaceableShoringPiece::InitializeFromSavedState(EShoringPieceType InType, FGuid InGuid,
    FName InAnchorId, const FTransform& InTransform, FVector InDimensionsCm, float InHealth)
{
    PieceType = InType;
    PieceGuid = InGuid.IsValid() ? InGuid : FGuid::NewGuid();
    AnchorId = InAnchorId;
    DimensionsCm = InDimensionsCm;
    StructuralHealth = FMath::Clamp(InHealth, 0.0f, 100.0f);
    SetActorTransform(InTransform);
    SetActorScale3D(FVector(
        FMath::Max(0.01f, DimensionsCm.X / 100.0f),
        FMath::Max(0.01f, DimensionsCm.Y / 100.0f),
        FMath::Max(0.01f, DimensionsCm.Z / 100.0f)));
    bValidatedPlacement = true;
}

void APlaceableShoringPiece::ApplyStructuralDamage(float Amount)
{
    if (!FMath::IsFinite(Amount) || Amount <= 0.0f || StructuralHealth <= 0.0f)
    {
        return;
    }
    StructuralHealth = FMath::Max(0.0f, StructuralHealth - Amount);
    if (StructuralHealth <= 0.0f && PieceMesh)
    {
        PieceMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        PieceMesh->SetSimulatePhysics(true);
    }
}
