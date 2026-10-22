#include "PlaceableShoringPiece.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "WorldCore/Classification/LandscapeBuilderSubsystem.h"

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
    bHasVoxelAnchor = Candidate.bHasVoxelAnchor;
    SupportAnchorWorldCm = Candidate.SupportAnchorWorldCm;
    SupportNormal = Candidate.SupportNormal.GetSafeNormal();
    RequiredAnchorIds = Candidate.RequiredAnchorIds;
    bSupportRecheckRequired = true;
    bValidatedPlacement = false;
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            Volume->TryWorldToCell(SupportAnchorWorldCm, ApproximateAnchorCell);
        }
    }
    if (Candidate.PieceType == EShoringPieceType::Upright
        || Candidate.PieceType == EShoringPieceType::WallBoard
        || Candidate.PieceType == EShoringPieceType::Strut)
    {
        Tags.AddUnique(TEXT("World.Shoring"));
    }
}

void APlaceableShoringPiece::InitializeFromSavedState(EShoringPieceType InType, FGuid InGuid,
    FName InAnchorId, const FTransform& InTransform, FVector InDimensionsCm, float InHealth,
    bool bInHasVoxelAnchor, FVector InSupportAnchorWorldCm, FVector InSupportNormal,
    const TArray<FName>& InRequiredAnchorIds)
{
    PieceType = InType;
    PieceGuid = InGuid.IsValid() ? InGuid : FGuid::NewGuid();
    AnchorId = InAnchorId;
    DimensionsCm = InDimensionsCm;
    StructuralHealth = FMath::Clamp(InHealth, 0.0f, 100.0f);
    bHasVoxelAnchor = bInHasVoxelAnchor;
    SupportAnchorWorldCm = InSupportAnchorWorldCm;
    SupportNormal = InSupportNormal.GetSafeNormal();
    RequiredAnchorIds = InRequiredAnchorIds;
    bSupportRecheckRequired = true;
    SetActorTransform(InTransform);
    if (UWorld* World = GetWorld())
    {
        if (UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>())
        {
            Volume->TryWorldToCell(SupportAnchorWorldCm, ApproximateAnchorCell);
        }
    }
    SetActorScale3D(FVector(
        FMath::Max(0.01f, DimensionsCm.X / 100.0f),
        FMath::Max(0.01f, DimensionsCm.Y / 100.0f),
        FMath::Max(0.01f, DimensionsCm.Z / 100.0f)));
    bValidatedPlacement = false;
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
        Tags.AddUnique(TEXT("World.State.Unsupported"));
        PieceMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        PieceMesh->SetSimulatePhysics(true);
    }
}


void APlaceableShoringPiece::MarkSupportDirty()
{
    bValidatedPlacement = false;
    bSupportRecheckRequired = true;
    Tags.AddUnique(TEXT("World.State.SupportNeedsRecheck"));
    Tags.Remove(TEXT("World.State.SupportValidated"));
}

bool APlaceableShoringPiece::RevalidateVoxelAnchor()
{
    UWorld* World = GetWorld();
    if (!World || StructuralHealth <= 0.0f)
    {
        MarkSupportDirty();
        return false;
    }

    for (const FName RequiredAnchor : RequiredAnchorIds)
    {
        bool bFoundValidAnchor = false;
        for (TActorIterator<APlaceableShoringPiece> It(World); It; ++It)
        {
            APlaceableShoringPiece* Other = *It;
            if (IsValid(Other) && Other != this && Other->AnchorId == RequiredAnchor
                && Other->IsStructurallyIntact() && Other->bValidatedPlacement)
            {
                bFoundValidAnchor = true;
                break;
            }
        }
        if (!bFoundValidAnchor)
        {
            MarkSupportDirty();
            return false;
        }
    }

    if (bHasVoxelAnchor)
    {
        UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>();
        if (!Volume)
        {
            MarkSupportDirty();
            return false;
        }
        const float ProbeDepth = FMath::Max(2.0f, Volume->CellSizeCm * 1.5f);
        const FVector ProbePoint = SupportAnchorWorldCm - SupportNormal.GetSafeNormal() * ProbeDepth;
        ULandscapeBuilderSubsystem* Builder = World->GetSubsystem<ULandscapeBuilderSubsystem>();
        float Density = 0.0f;
        if (!Builder || Builder->QueryModeAtWorld(ProbePoint).Mode != EWorldTerrainMode::EditableVoxel
            || !Volume->TrySampleDensityAtWorld(ProbePoint, Density) || Density < 0.5f)
        {
            MarkSupportDirty();
            return false;
        }
    }
    else if (RequiredAnchorIds.IsEmpty())
    {
        // An unattached strut/platform has no voxel anchor or connected support.
        MarkSupportDirty();
        return false;
    }

    bValidatedPlacement = true;
    bSupportRecheckRequired = false;
    Tags.Remove(TEXT("World.State.SupportNeedsRecheck"));
    Tags.AddUnique(TEXT("World.State.SupportValidated"));
    return true;
}
