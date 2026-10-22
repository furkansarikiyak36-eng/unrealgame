// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlaceableShoringPiece.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    const FName ShoringPieceTag(TEXT("World.Object.ShoringPiece"));
}

APlaceableShoringPiece::APlaceableShoringPiece()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(false);
    Tags.AddUnique(ShoringPieceTag);

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ShoringCollision"));
    SetRootComponent(CollisionBox);
    CollisionBox->SetBoxExtent(HalfExtentsCm);
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Block);
    CollisionBox->SetGenerateOverlapEvents(false);
    CollisionBox->SetSimulatePhysics(false);

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShoringVisual"));
    VisualMesh->SetupAttachment(CollisionBox);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetGenerateOverlapEvents(false);
    VisualMesh->SetMobility(EComponentMobility::Movable);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultCube(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (DefaultCube.Succeeded())
    {
        VisualMesh->SetStaticMesh(DefaultCube.Object);
    }
    VisualMesh->SetRelativeScale3D(HalfExtentsCm * 0.02f);
}

bool APlaceableShoringPiece::InitializeFromSpec(const FShoringPieceSpec& Spec, bool bAsPreview)
{
    FString Error;
    if (!Spec.Validate(Error))
    {
        return false;
    }

    SetActorTransform(Spec.Transform);
    PieceId = Spec.PieceId;
    PlanId = Spec.PlanId;
    PieceKind = Spec.Kind;
    SupportAnchor = Spec.SupportAnchor;
    ConnectedPieceIds = Spec.ConnectedPieceIds;
    HalfExtentsCm = Spec.HalfExtentsCm;
    DesignLoad = Spec.DesignLoad;
    Health = MaxHealth;
    LastDamageCause = EShoringDamageCause::Manual;

    CollisionBox->SetBoxExtent(HalfExtentsCm);
    VisualMesh->SetRelativeScale3D(HalfExtentsCm * 0.02f);
    VisualMesh->SetRenderCustomDepth(bAsPreview);
    if (bAsPreview)
    {
        VisualMesh->SetCustomDepthStencilValue(2);
    }
    SetPieceState(bAsPreview ? EShoringPieceState::Planned : EShoringPieceState::Active);
    return true;
}

bool APlaceableShoringPiece::ApplyDamage(float Amount, EShoringDamageCause Cause, FString& OutError)
{
    OutError.Reset();
    if (!FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        OutError = TEXT("Damage amount must be finite and positive.");
        return false;
    }
    if (PieceState != EShoringPieceState::Active
        && PieceState != EShoringPieceState::Damaged
        && PieceState != EShoringPieceState::Warning
        && PieceState != EShoringPieceState::Overloaded)
    {
        OutError = TEXT("Only an active, damaged or overloaded shoring piece can take new damage.");
        return false;
    }

    Health = FMath::Clamp(Health - Amount, 0.0f, MaxHealth);
    LastDamageCause = Cause;
    if (Health <= KINDA_SMALL_NUMBER)
    {
        SetPieceState(EShoringPieceState::Failed);
    }
    else if (Health <= MaxHealth * 0.7f)
    {
        SetPieceState(EShoringPieceState::Damaged);
    }
    else
    {
        SetPieceState(EShoringPieceState::Active);
    }
    return true;
}

bool APlaceableShoringPiece::Repair(float Amount, FString& OutError)
{
    OutError.Reset();
    if (!FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        OutError = TEXT("Repair amount must be finite and positive.");
        return false;
    }
    if (PieceState != EShoringPieceState::Damaged || Health <= KINDA_SMALL_NUMBER)
    {
        OutError = TEXT("Only a damaged, non-failed shoring piece can be repaired. Failed pieces must be replaced.");
        return false;
    }

    Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
    if (Health >= MaxHealth - KINDA_SMALL_NUMBER)
    {
        Health = MaxHealth;
        SetPieceState(EShoringPieceState::Active);
    }
    return true;
}

void APlaceableShoringPiece::SetPieceState(EShoringPieceState NewState)
{
    PieceState = NewState;
    RefreshCollisionState();
}

bool APlaceableShoringPiece::UpdateSupportAnchor(const FShoringSupportAnchor& NewAnchor)
{
    if (!NewAnchor.IsFinite())
    {
        return false;
    }
    SupportAnchor = NewAnchor;
    return true;
}

void APlaceableShoringPiece::UpdateConnections(const TArray<FGuid>& NewConnectedPieceIds)
{
    ConnectedPieceIds.Reset();
    for (const FGuid& ConnectedId : NewConnectedPieceIds)
    {
        if (ConnectedId.IsValid() && ConnectedId != PieceId)
        {
            ConnectedPieceIds.AddUnique(ConnectedId);
        }
    }
}

void APlaceableShoringPiece::RefreshCollisionState()
{
    const bool bShouldSimulateCollapse = PieceState == EShoringPieceState::Unsupported;
    const bool bShouldCollide =
        PieceState == EShoringPieceState::Active
        || PieceState == EShoringPieceState::Damaged
        || PieceState == EShoringPieceState::Warning
        || PieceState == EShoringPieceState::Overloaded
        || bShouldSimulateCollapse;

    CollisionBox->SetCollisionEnabled(
        bShouldCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    CollisionBox->SetEnableGravity(bShouldSimulateCollapse);
    CollisionBox->SetSimulatePhysics(bShouldSimulateCollapse);
    VisualMesh->SetRenderCustomDepth(
        PieceState == EShoringPieceState::Planned
        || PieceState == EShoringPieceState::Failed);
    if (PieceState != EShoringPieceState::Planned && PieceState != EShoringPieceState::Failed)
    {
        VisualMesh->SetCustomDepthStencilValue(0);
    }
}