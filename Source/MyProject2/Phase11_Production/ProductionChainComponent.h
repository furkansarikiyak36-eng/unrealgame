#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProductionChainTypes.h"
#include "ProductionChainComponent.generated.h"

class UInventoryComponent;
class UProductionRecipeAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProductionJobChanged, FGuid, JobId, EProductionJobStatus, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProductionQueueChanged, FName, StationKey);

/**
 * Queue orchestration for a production station.
 * Input/output inventory components remain the only stock owners.
 * Power and worker availability are supplied by the owning actor/Blueprint or a future shared utility grid.
 */
UCLASS(ClassGroup=(Production), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UProductionChainComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UProductionChainComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintPure, Category="Production")
    FName GetStableStationKey() const;

    UFUNCTION(BlueprintPure, Category="Production|Queue")
    int32 GetActiveJobCount() const;

    UFUNCTION(BlueprintPure, Category="Production|Queue")
    TArray<FProductionJob> GetJobs() const { return Jobs; }

    UFUNCTION(BlueprintPure, Category="Production|Queue")
    bool IsQueuePaused() const { return bQueuePaused; }

    UFUNCTION(BlueprintCallable, Category="Production|Queue")
    bool QueueRecipe(UProductionRecipeAsset* Recipe, int32 BatchCount = 1);

    UFUNCTION(BlueprintCallable, Category="Production|Queue")
    bool QueueRecipeByID(FName RecipeID, int32 BatchCount = 1);

    UFUNCTION(BlueprintCallable, Category="Production|Queue")
    bool QueueConfiguredRecipe(FName RecipeID, const TArray<FProductionIngredient>& Inputs, const TArray<FProductionOutput>& Outputs, float DurationSeconds, int32 BatchCount = 1, float RequiredPowerWatts = 0.0f, int32 RequiredWorkers = 0);

    /** Built-in example route: 1 HalfLog -> 2 Plank; 5 seconds, 100 W, one worker per batch. */
    UFUNCTION(BlueprintCallable, Category="Production|Sawmill")
    bool QueueStandardPlankBatch(int32 BatchCount = 1);

    UFUNCTION(BlueprintCallable, Category="Production|Queue")
    bool CancelJob(const FGuid& JobId);

    UFUNCTION(BlueprintCallable, Category="Production|Queue")
    void PauseQueue();

    UFUNCTION(BlueprintCallable, Category="Production|Queue")
    void ResumeQueue();

    UFUNCTION(BlueprintCallable, Category="Production|Queue")
    void ClearCompletedHistory();

    UFUNCTION(BlueprintCallable, Category="Production|Power")
    bool SetAvailablePowerWatts(float InAvailablePowerWatts);

    UFUNCTION(BlueprintCallable, Category="Production|Workers")
    bool SetAvailableWorkers(int32 InAvailableWorkers);

    UFUNCTION(BlueprintCallable, Category="Production|Persistence")
    bool ExportSnapshot(FProductionChainSnapshot& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category="Production|Persistence")
    bool ValidateSnapshot(const FProductionChainSnapshot& Snapshot) const;

    UFUNCTION(BlueprintCallable, Category="Production|Persistence")
    bool ImportSnapshot(const FProductionChainSnapshot& Snapshot);

    UPROPERTY(BlueprintAssignable, Category="Production|Events")
    FOnProductionJobChanged OnJobChanged;

    UPROPERTY(BlueprintAssignable, Category="Production|Events")
    FOnProductionQueueChanged OnQueueChanged;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Production|Configuration")
    FName StationKey = NAME_None;

    /** Must reference existing inventory owners. Null input falls back to the owner's inventory. */
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Production|Inventory")
    TObjectPtr<UInventoryComponent> InputInventory = nullptr;

    /** Null output uses the resolved input inventory. Use InventoryStorageActor inventories for core-save persistence. */
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Production|Inventory")
    TObjectPtr<UInventoryComponent> OutputInventory = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production|Recipes")
    TArray<TObjectPtr<UProductionRecipeAsset>> Recipes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Production|Configuration", meta=(ClampMin="1", ClampMax="32"))
    int32 QueueCapacity = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Production|Configuration", meta=(ClampMin="1", ClampMax="128"))
    int32 MaxRetainedJobs = 32;

    /** Live power availability in watts, supplied externally; not persisted as a resource balance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Production|Power", meta=(ClampMin="0.0"))
    float AvailablePowerWatts = 0.0f;

    /** Live worker availability supplied by the station/AI layer. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Production|Workers", meta=(ClampMin="0", ClampMax="1000"))
    int32 AvailableWorkers = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Production|Configuration")
    bool bQueueStartsPaused = false;

protected:
    UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, Category="Production|Queue")
    TArray<FProductionJob> Jobs;

    UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, Category="Production|Queue")
    bool bQueuePaused = false;

    UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, Category="Production|Queue")
    int64 Revision = 0;

    UInventoryComponent* ResolveInputInventory() const;
    UInventoryComponent* ResolveOutputInventory() const;
    UProductionRecipeAsset* FindUniqueRecipe(FName RecipeID) const;
    bool EnqueuePlan(FName RecipeID, const FText& DisplayName,
        const TArray<FProductionIngredient>& Inputs, const TArray<FProductionOutput>& Outputs,
        float BaseDurationSeconds, float RequiredPowerWatts, int32 RequiredWorkers, int32 BatchCount);
    void TrimCompletedHistory();
    void NotifyStateChanged(const FGuid& JobId = FGuid(), EProductionJobStatus Status = EProductionJobStatus::Queued);
    static bool IsActiveStatus(EProductionJobStatus Status);
    static bool IsValidSavedJob(const FProductionJob& Job);
    bool UsesCorePersistedInventories() const;

    static constexpr int32 MaxBatchCount = 100;
    static constexpr int32 MaxDeltaSecondsPerTick = 5;
};
