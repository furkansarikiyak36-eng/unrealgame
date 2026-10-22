#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCore/Events/WorldMechanicEventTypes.h"
#include "WorldMechanicEventSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldMechanicEvent, const FWorldMechanicEvent&, Event);

/**
 * Bounded event queue for cross-system notifications. Producers enqueue facts;
 * consumers decide how to react. It does not make an event authoritative by itself.
 */
UCLASS()
class MYPROJECT2_API UWorldMechanicEventSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    UFUNCTION(BlueprintCallable, Category = "World|Events")
    bool EnqueueEvent(FWorldMechanicEvent Event);

    UFUNCTION(BlueprintCallable, Category = "World|Events")
    bool PublishImmediate(FWorldMechanicEvent Event);

    UFUNCTION(BlueprintCallable, Category = "World|Events")
    void ClearTransientEvents();

    UFUNCTION(BlueprintPure, Category = "World|Events")
    int32 GetPendingEventCount() const { return PendingEvents.Num(); }

    UFUNCTION(BlueprintPure, Category = "World|Events")
    uint32 GetWorldVersion() const { return WorldVersion; }

    UPROPERTY(BlueprintAssignable, Category = "World|Events")
    FOnWorldMechanicEvent OnWorldMechanicEvent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Events", meta = (ClampMin = "1", ClampMax = "256"))
    int32 MaxEventsPerTick = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Events", meta = (ClampMin = "64", ClampMax = "65536"))
    int32 MaxQueuedEvents = 4096;

private:
    TArray<FWorldMechanicEvent> PendingEvents;
    uint32 WorldVersion = 0;

    void Dispatch(const FWorldMechanicEvent& Event);
};
