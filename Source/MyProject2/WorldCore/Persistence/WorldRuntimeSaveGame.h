#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WorldCore/ExcavationTypes.h"
#include "Shoring/PlaceableShoringPiece.h"
#include "SurvivalComponent.h"
#include "WorldRuntimeSaveGame.generated.h"

USTRUCT(BlueprintType)
struct MYPROJECT2_API FSavedShoringPieceRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    FGuid PieceGuid;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    EShoringPieceType PieceType = EShoringPieceType::Upright;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    FName AnchorId = NAME_None;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    FTransform Transform = FTransform::Identity;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    FVector DimensionsCm = FVector(20.0f, 20.0f, 100.0f);

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    float StructuralHealth = 100.0f;
};

UCLASS()
class MYPROJECT2_API UWorldRuntimeSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    int32 SchemaVersion = 1;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    FExcavationVolumeSnapshot ExcavationSnapshot;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    TArray<FSavedShoringPieceRecord> ShoringPieces;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    bool bHasPlayerTransform = false;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    FTransform PlayerTransform = FTransform::Identity;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    bool bHasSurvivalState = false;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
    FSurvivalStateSnapshot SurvivalState;
};
