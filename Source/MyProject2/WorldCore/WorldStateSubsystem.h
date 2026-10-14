#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldTypes.h"
#include "ChunkCoordinate.h"
#include "ChunkStateStore.h"
#include "WorldStateSubsystem.generated.h"
class USimulationBudgetAllocator; class UWorldChunkSubsystem; class UWorldEventSubsystem; class UWaterSimulation; class USoilSimulation; class UFireSimulation; class UDecaySimulation; class UChemistrySimulation; class UEventChainTracker;
UCLASS()
class MYPROJECT2_API UWorldStateSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    UFUNCTION(BlueprintCallable, Category="World") UChunkState* GetOrCreateChunkAtWorld(const FVector& WorldPos);
    UFUNCTION(BlueprintPure, Category="World") UChunkState* GetChunk(const FIntPoint& Coord) const;
    FChunkCell* GetCellAtWorld(const FVector& WorldPos, const FIntPoint& CellInChunk=FIntPoint(-1,-1));
    UFUNCTION(BlueprintCallable, Category="World") TArray<UChunkState*> GetChunksInRadius(const FVector& Center, float RadiusMeters) const;
    UFUNCTION(BlueprintPure, Category="World") FWorldTime GetWorldTime() const { return WorldTime; }
    UFUNCTION(BlueprintCallable, Category="World") void AdvanceWorldTime(float DeltaDays);
    UFUNCTION(BlueprintCallable, Category="World") void SynchronizeAfterLoad();
    UFUNCTION(BlueprintCallable, Category="World") void RequestChunkSimulation(UChunkState* Chunk, float ElapsedDays);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config") FWorldChunkSettings ChunkSettings;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="State") TObjectPtr<UChunkStateStore> StateStore;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="State") FWorldTime WorldTime;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation") float GameHoursPerRealSecond=1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation") int32 MaxChunksSimulatedPerFrame=8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Simulation") float DefaultTemperatureC=20.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") float TemperatureC=20.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") float TempMinC=15.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") float TempMaxC=25.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") float RainfallMm=0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") float WindSpeed=0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") bool bIsRaining=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") bool bIsSnowing=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") int32 RandomSeed=12345;
protected:
    UPROPERTY() TObjectPtr<USimulationBudgetAllocator> BudgetAllocator;
    UPROPERTY() TObjectPtr<UWaterSimulation> WaterSimulation;
    UPROPERTY() TObjectPtr<USoilSimulation> SoilSimulation;
    UPROPERTY() TObjectPtr<UFireSimulation> FireSimulation;
    UPROPERTY() TObjectPtr<UDecaySimulation> DecaySimulation;
    UPROPERTY() TObjectPtr<UChemistrySimulation> ChemistrySimulation;
    UPROPERTY() TObjectPtr<UEventChainTracker> EventTracker;
    double HourAccumulator=0.0;
    int32 SimulationCursor=0;
    void UpdateWorldTimeTick(float DeltaTime);
    float ComputeRainfallForDay(int32 DayNumber) const;
    float ComputeRainfallSince(float LastWeatherDay, int32 CurrentDayIndex) const;
    void SimulateChunk(UChunkState* Chunk, float DeltaDays, float TargetDay=-1.0f);
};
