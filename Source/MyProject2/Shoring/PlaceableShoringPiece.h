#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlaceableShoringPiece.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EShoringPieceType : uint8
{
    Upright UMETA(DisplayName = "Upright"),
    WallBoard UMETA(DisplayName = "Wall Board / Lagging"),
    Strut UMETA(DisplayName = "Cross Strut"),
    Platform UMETA(DisplayName = "Platform"),
    Ladder UMETA(DisplayName = "Ladder")
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FShoringPlacementCandidate
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Shoring")
    FGuid CandidateId;

    UPROPERTY(BlueprintReadOnly, Category = "Shoring")
    FName AnchorId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Shoring")
    EShoringPieceType PieceType = EShoringPieceType::Upright;

    UPROPERTY(BlueprintReadOnly, Category = "Shoring")
    FTransform Transform = FTransform::Identity;

    /** Full dimensions in centimetres, before scale is applied to the 100 cm engine cube. */
    UPROPERTY(BlueprintReadOnly, Category = "Shoring")
    FVector DimensionsCm = FVector(20.0f, 20.0f, 100.0f);

    UPROPERTY(BlueprintReadOnly, Category = "Shoring")
    bool bPlanValid = false;

    UPROPERTY(BlueprintReadOnly, Category = "Shoring|Support")
    bool bHasVoxelAnchor = false;

    UPROPERTY(BlueprintReadOnly, Category = "Shoring|Support")
    FVector SupportAnchorWorldCm = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Shoring|Support")
    FVector SupportNormal = FVector::UpVector;

    UPROPERTY(BlueprintReadOnly, Category = "Shoring|Support")
    TArray<FName> RequiredAnchorIds;

    UPROPERTY(BlueprintReadOnly, Category = "Shoring")
    FString Message;
};

UCLASS()
class MYPROJECT2_API APlaceableShoringPiece : public AActor
{
    GENERATED_BODY()
public:
    APlaceableShoringPiece();

    UFUNCTION(BlueprintCallable, Category = "Shoring")
    void InitializeFromCandidate(const FShoringPlacementCandidate& Candidate);

    UFUNCTION(BlueprintCallable, Category = "Shoring")
    void InitializeFromSavedState(EShoringPieceType InType, FGuid InGuid, FName InAnchorId,
        const FTransform& InTransform, FVector InDimensionsCm, float InHealth,
        bool bInHasVoxelAnchor, FVector InSupportAnchorWorldCm, FVector InSupportNormal,
        const TArray<FName>& InRequiredAnchorIds);

    UFUNCTION(BlueprintCallable, Category = "Shoring")
    void ApplyStructuralDamage(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Shoring|Support")
    void MarkSupportDirty();

    UFUNCTION(BlueprintCallable, Category = "Shoring|Support")
    bool RevalidateVoxelAnchor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring|Support")
    bool bHasVoxelAnchor = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring|Support")
    FVector SupportAnchorWorldCm = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring|Support")
    FVector SupportNormal = FVector::UpVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring|Support")
    TArray<FName> RequiredAnchorIds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring|Support")
    bool bSupportRecheckRequired = true;

    UFUNCTION(BlueprintPure, Category = "Shoring")
    bool IsStructurallyIntact() const { return StructuralHealth > 0.0f; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shoring")
    TObjectPtr<UStaticMeshComponent> PieceMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring")
    FGuid PieceGuid;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring")
    EShoringPieceType PieceType = EShoringPieceType::Upright;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring")
    FName AnchorId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Shoring")
    FVector DimensionsCm = FVector(20.0f, 20.0f, 100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Shoring", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float StructuralHealth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shoring")
    bool bValidatedPlacement = false;
};
