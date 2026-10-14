#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCore/ExcavationTypes.h"
#include "ExcavationTerrainChunkSubsystem.generated.h"

class AExcavationTerrainChunkActor;
class UExcavationFlatBaseDensitySource;
class UExcavationVolumeSubsystem;

/**
 * Routes excavation change/reset events to the registered 3D mesh adapters.
 * This is a derived renderer registry, not a second world/chunk state owner.
 */
UCLASS()
class MYPROJECT2_API UExcavationTerrainChunkSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void RegisterChunkActor(AExcavationTerrainChunkActor* Actor);
    void UnregisterChunkActor(AExcavationTerrainChunkActor* Actor);

protected:
    UFUNCTION()
    void HandleVolumeChanged(const FExcavationEditRecord& Record, bool bIsRollback);

    UFUNCTION()
    void HandleVolumeReset();

    UPROPERTY(Transient)
    TObjectPtr<UExcavationFlatBaseDensitySource> FlatFallbackDensitySource;

    TMap<FIntVector, TArray<TWeakObjectPtr<AExcavationTerrainChunkActor>>> RegisteredChunks;
};
