#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "SaveGameSubsystem.generated.h"
class ALogActor; class ULogDefinition;
USTRUCT(BlueprintType) struct FLogSaveData
{
    GENERATED_BODY()
    UPROPERTY() FTransform Transform = FTransform::Identity;
    UPROPERTY() TObjectPtr<ULogDefinition> Definition = nullptr;
    UPROPERTY() uint8 State = 0;
    UPROPERTY() float Health = 100.0f;
    UPROPERTY() float CurrentLength = 65.0f;
    UPROPERTY() uint8 Shape = 0;
    UPROPERTY() FName InfrastructureLinkKey = NAME_None;
};
UCLASS() class MYPROJECT2_API ULogSaveGame:public USaveGame
{GENERATED_BODY() public: UPROPERTY() TArray<FLogSaveData> Logs; };
UCLASS() class MYPROJECT2_API USaveGameSubsystem:public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) bool SaveWorld(const FString& SlotName);
    UFUNCTION(BlueprintCallable) bool LoadWorld(const FString& SlotName);
    UPROPERTY() TArray<FLogSaveData> CachedLogs;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Save") TSubclassOf<ALogActor> LogClass;
};
