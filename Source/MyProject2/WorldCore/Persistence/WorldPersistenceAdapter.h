#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldPersistenceAdapter.generated.h"

class UWorldRuntimeSaveGame;

UCLASS()
class MYPROJECT2_API UWorldPersistenceAdapter : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "World|Persistence")
    bool SaveWorldToSlot(const FString& SlotName, int32 UserIndex, FString& OutFailure);

    UFUNCTION(BlueprintCallable, Category = "World|Persistence")
    bool LoadWorldFromSlot(const FString& SlotName, int32 UserIndex, FString& OutFailure);
};
