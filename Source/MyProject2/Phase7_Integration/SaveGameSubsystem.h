#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "ForestTypes.h"
#include "Core/StructuralMaterialTypes.h"
#include "CarvableLog.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneMasonryActor.h"
#include "SaveGameSubsystem.generated.h"

class ALogActor;
class ULogDefinition;
class UStaticMesh;
class UStoneDefinition;

USTRUCT(BlueprintType)
struct FLogSaveData
{
    GENERATED_BODY()

    UPROPERTY(SaveGame) FTransform Transform = FTransform::Identity;
    UPROPERTY(SaveGame) TObjectPtr<ULogDefinition> Definition = nullptr;
    UPROPERTY(SaveGame) uint8 State = 0;
    UPROPERTY(SaveGame) float Health = 100.0f;
    UPROPERTY(SaveGame) float CurrentLength = 65.0f;
    UPROPERTY(SaveGame) uint8 Shape = 0;
};

USTRUCT(BlueprintType)
struct FCarvableLogSaveData
{
    GENERATED_BODY()

    UPROPERTY(SaveGame) FTransform Transform = FTransform::Identity;
    UPROPERTY(SaveGame) TObjectPtr<UStaticMesh> SourceMesh = nullptr;
    UPROPERTY(SaveGame) float Damage = 0.0f;
    UPROPERTY(SaveGame) bool bAnchored = false;
    UPROPERTY(SaveGame) bool bTemporary = false;

    bool Validate(FString& OutError) const;
};

USTRUCT(BlueprintType)
struct FStoneActorSaveData
{
    GENERATED_BODY()

    UPROPERTY(SaveGame) FTransform Transform = FTransform::Identity;
    UPROPERTY(SaveGame) TObjectPtr<UStoneDefinition> BaseDefinition = nullptr;
    UPROPERTY(SaveGame) TSubclassOf<AStoneActor> ActorClass;
    UPROPERTY(SaveGame) float Health = 100.0f;
    UPROPERTY(SaveGame) EStoneShape Shape = EStoneShape::Building;
    UPROPERTY(SaveGame) EStructuralNodeState State = EStructuralNodeState::Loose;
    UPROPERTY(SaveGame) FVector Dimensions = FVector(30.0f, 20.0f, 20.0f);

    bool Validate(FString& OutError) const;
};

USTRUCT(BlueprintType)
struct FStoneMasonrySaveData
{
    GENERATED_BODY()

    UPROPERTY(SaveGame) FTransform Transform = FTransform::Identity;
    UPROPERTY(SaveGame) TObjectPtr<UStaticMesh> Mesh = nullptr;
    UPROPERTY(SaveGame) TSubclassOf<AStoneMasonryActor> ActorClass;
    UPROPERTY(SaveGame) EMasonryType MasonryType = EMasonryType::DryStone;
    UPROPERTY(SaveGame) bool bHasThroughStones = true;
    UPROPERTY(SaveGame) bool bHasHearting = true;
    UPROPERTY(SaveGame) float BatterRatio = 0.125f;
    UPROPERTY(SaveGame) bool bHasCrossingJoints = true;
    UPROPERTY(SaveGame) float CompressiveStrengthMPa = 25.0f;
    UPROPERTY(SaveGame) float StructuralHealth = 200.0f;
    UPROPERTY(SaveGame) float SelfWeight = 250.0f;
    UPROPERTY(SaveGame) float CompressiveCapacityKN = 2500.0f;
    UPROPERTY(SaveGame) bool bIsGrounded = true;

    bool Validate(FString& OutError) const;
};

UCLASS()
class MYPROJECT2_API ULogSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame) TArray<FLogSaveData> Logs;
    UPROPERTY(SaveGame) TArray<FCarvableLogSaveData> CarvableLogs;
    UPROPERTY(SaveGame) TArray<FStoneActorSaveData> Stones;
    UPROPERTY(SaveGame) TArray<FStoneMasonrySaveData> StoneMasonry;
    UPROPERTY(SaveGame) bool bHasPlayerState = false;
    UPROPERTY(SaveGame) FTransform PlayerTransform = FTransform::Identity;
    UPROPERTY(SaveGame) TArray<uint8> InventoryItemIds;
    UPROPERTY(SaveGame) TArray<int32> InventoryCounts;
};

UCLASS()
class MYPROJECT2_API USaveGameSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Save") bool SaveWorld(const FString& SlotName);
    UFUNCTION(BlueprintCallable, Category = "Save") bool LoadWorld(const FString& SlotName);
    UFUNCTION(BlueprintPure, Category = "Save") FString GetLastOperationError() const { return LastOperationError; }

    UPROPERTY() TArray<FLogSaveData> CachedLogs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save") TSubclassOf<ALogActor> LogClass;

private:
    FString LastOperationError;
};
