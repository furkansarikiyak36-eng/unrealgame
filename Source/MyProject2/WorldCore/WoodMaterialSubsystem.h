#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WoodMaterialSubsystem.generated.h"

class ACarvableLog;
class UWorldStateSubsystem;

/**
 * Budgeted adapter from shared world weather/time to the existing ACarvableLog
 * material-condition fields. It does not own fire state or a structural graph.
 */
UCLASS()
class MYPROJECT2_API UWoodMaterialSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintCallable, Category = "Material|Wood")
    void RefreshTrackedLogs();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material|Wood|Budget")
    int32 MaxLogsProcessedPerTick = 24;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material|Wood|Budget")
    float RefreshIntervalSeconds = 2.0f;

    UFUNCTION(BlueprintPure, Category = "Material|Wood")
    int32 GetTrackedLogCount() const { return TrackedLogs.Num(); }

    UFUNCTION(BlueprintPure, Category = "Material|Wood")
    int32 GetLastProcessedLogCount() const { return LastProcessedLogCount; }

private:
    UWorldStateSubsystem* GetWorldState() const;
    bool IsExposedToRain(const ACarvableLog* Log) const;
    void BeginWeatherBatch(float TargetWorldDay, UWorldStateSubsystem* WorldState);

    TArray<TWeakObjectPtr<ACarvableLog>> TrackedLogs;
    int32 BatchCursor = 0;
    int32 LastProcessedLogCount = 0;

    bool bBatchActive = false;
    float BatchTargetWorldDay = 0.0f;
    float BatchElapsedDays = 0.0f;
    float BatchTemperatureC = 20.0f;
    float BatchRainfallMm = 0.0f;
    bool bBatchIsRaining = false;

    float LastCompletedWorldDay = 0.0f;
    double LastRefreshTimeSeconds = -1.0;
};
