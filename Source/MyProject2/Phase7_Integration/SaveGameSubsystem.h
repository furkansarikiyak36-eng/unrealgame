#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "WorldCore/ExcavationTypes.h"
#include "Shoring/ShoringTypes.h"
#include "SurvivalComponent.h"
#include "Survival/UndergroundExposureComponent.h"
#include "SaveGameSubsystem.generated.h"

class ALogActor;
class ULogDefinition;
class AForestCharacter;

USTRUCT(BlueprintType)
struct FLogSaveData
{
    GENERATED_BODY()

    UPROPERTY() FTransform Transform = FTransform::Identity;
    UPROPERTY() TObjectPtr<ULogDefinition> Definition = nullptr;
    UPROPERTY() uint8 State = 0;
    UPROPERTY() float Health = 100.0f;
    UPROPERTY() float CurrentLength = 65.0f;
    UPROPERTY() uint8 Shape = 0;
};

UCLASS()
class MYPROJECT2_API ULogSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() TArray<FLogSaveData> Logs;
};

/**
 * Versioned full-loop world save. Subclasses the old log-only save type so
 * older slots can still be opened by USaveGameSubsystem::LoadWorld.
 */
UCLASS()
class MYPROJECT2_API UWorldSaveGame : public ULogSaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 SaveSchemaVersion = 2;
    UPROPERTY(SaveGame) FName MapPackageName = NAME_None;
    UPROPERTY(SaveGame) FExcavationVolumeSnapshot Excavation;
    UPROPERTY(SaveGame) FShoringWorldSnapshot Shoring;
    UPROPERTY(SaveGame) bool bHasPlayerState = false;
    UPROPERTY(SaveGame) FTransform PlayerTransform = FTransform::Identity;
    UPROPERTY(SaveGame) FTransform RespawnTransform = FTransform::Identity;
    UPROPERTY(SaveGame) TArray<uint8> InventoryItemIds;
    UPROPERTY(SaveGame) TArray<int32> InventoryCounts;
    UPROPERTY(SaveGame) FSurvivalComponentSnapshot Survival;
    UPROPERTY(SaveGame) bool bHasExposureState = false;
    UPROPERTY(SaveGame) FSurvivalExposureSnapshot Exposure;
};

UCLASS()
class MYPROJECT2_API USaveGameSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Save")
    bool SaveWorld(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Save")
    bool LoadWorld(const FString& SlotName);

    UFUNCTION(BlueprintPure, Category = "Save")
    const FString& GetLastOperationError() const { return LastOperationError; }

    UPROPERTY()
    TArray<FLogSaveData> CachedLogs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TSubclassOf<ALogActor> LogClass;

private:
    bool LoadLegacyLogs(ULogSaveGame* Save, UWorld* World, FString& OutError);
    bool LoadCompleteWorld(UWorldSaveGame* Save, UWorld* World, FString& OutError);

    FString LastOperationError;
};
