#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RegionSeasonTypes.h"
#include "RegionSeasonSubsystem.generated.h"

class AWorldRegionVolume;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWorldSeasonChanged, EWorldSeason, PreviousSeason, EWorldSeason, NewSeason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRegionSeasonDay, int32, DayOfYear, EWorldSeason, Season, int32, WorldDay);

/**
 * Owns the season calendar and resolves world positions to region volumes.
 * Regional climate is exposed as data; existing weather and world simulation are not silently overridden.
 */
UCLASS()
class MYPROJECT2_API URegionSeasonSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calendar", meta=(ClampMin="4", ClampMax="1000")) int32 DaysPerYear = 360;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calendar", meta=(ClampMin="1", ClampMax="500")) int32 DaysPerSeason = 90;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calendar") int32 InitialWorldDay = 1;

    UPROPERTY(BlueprintAssignable, Category="Events") FOnWorldSeasonChanged OnSeasonChanged;
    UPROPERTY(BlueprintAssignable, Category="Events") FOnRegionSeasonDay OnSeasonDayAdvanced;

    UFUNCTION(BlueprintPure, Category="Calendar") EWorldSeason GetCurrentSeason() const { return CurrentSeason; }
    UFUNCTION(BlueprintPure, Category="Calendar") int32 GetCurrentWorldDay() const { return CurrentWorldDay; }
    UFUNCTION(BlueprintPure, Category="Calendar") int32 GetCurrentDayOfYear() const;
    UFUNCTION(BlueprintPure, Category="Calendar") EWorldSeason GetSeasonForWorldDay(int32 WorldDay) const;
    UFUNCTION(BlueprintCallable, Category="Region") FRegionClimateSnapshot GetClimateAtLocation(const FVector& WorldLocation) const;
    UFUNCTION(BlueprintPure, Category="Region") AWorldRegionVolume* GetRegionAtLocation(const FVector& WorldLocation) const;

    void RegisterRegion(AWorldRegionVolume* Region);
    void UnregisterRegion(AWorldRegionVolume* Region);

protected:
    UPROPERTY() TArray<TObjectPtr<AWorldRegionVolume>> RegisteredRegions;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Calendar") EWorldSeason CurrentSeason = EWorldSeason::Spring;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Calendar") int32 CurrentWorldDay = 1;

    UFUNCTION()
    void HandleSimulationDayTick(int32 DayNumber);

private:
    void UpdateCalendar(int32 NewWorldDay, bool bBroadcast);
};
