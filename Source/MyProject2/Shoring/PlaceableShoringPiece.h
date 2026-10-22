// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shoring/ShoringTypes.h"
#include "PlaceableShoringPiece.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * One separate, GUID-addressable shoring member. It is not merged into voxel terrain.
 * The default cube mesh is an intentionally simple greybox; projects can assign timber meshes in a BP subclass.
 */
UCLASS(BlueprintType)
class MYPROJECT2_API APlaceableShoringPiece : public AActor
{
    GENERATED_BODY()

public:
    APlaceableShoringPiece();

    bool InitializeFromSpec(const FShoringPieceSpec& Spec, bool bAsPreview);

    UFUNCTION(BlueprintPure, Category = "Shoring")
    FGuid GetPieceId() const { return PieceId; }

    UFUNCTION(BlueprintPure, Category = "Shoring")
    FGuid GetPlanId() const { return PlanId; }

    UFUNCTION(BlueprintPure, Category = "Shoring")
    EShoringPieceKind GetPieceKind() const { return PieceKind; }

    UFUNCTION(BlueprintPure, Category = "Shoring")
    EShoringPieceState GetPieceState() const { return PieceState; }

    uint64 GetSourceDensityRevision() const { return SupportAnchor.SourceDensityRevision; }

    const FShoringSupportAnchor& GetSupportAnchor() const { return SupportAnchor; }
    const TArray<FGuid>& GetConnectedPieceIds() const { return ConnectedPieceIds; }
    const FVector& GetHalfExtentsCm() const { return HalfExtentsCm; }

    void SetPieceState(EShoringPieceState NewState);
    bool UpdateSupportAnchor(const FShoringSupportAnchor& NewAnchor);
    void UpdateConnections(const TArray<FGuid>& NewConnectedPieceIds);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shoring")
    TObjectPtr<UBoxComponent> CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shoring")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring|Identity")
    FGuid PieceId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring|Identity")
    FGuid PlanId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring")
    EShoringPieceKind PieceKind = EShoringPieceKind::Upright;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring")
    EShoringPieceState PieceState = EShoringPieceState::Planned;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring")
    FShoringSupportAnchor SupportAnchor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring")
    TArray<FGuid> ConnectedPieceIds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring")
    FVector HalfExtentsCm = FVector(10.0f);

private:
    void RefreshCollisionState();
};