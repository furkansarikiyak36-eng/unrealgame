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
    const bool bShouldCollide = PieceState == EShoringPieceState::Active;
    CollisionBox->SetCollisionEnabled(
        bShouldCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    VisualMesh->SetRenderCustomDepth(PieceState == EShoringPieceState::Planned);
    if (PieceState != EShoringPieceState::Planned)
    {
        VisualMesh->SetCustomDepthStencilValue(0);
    }
}