#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/DynamicMeshComponent.h"
#include "ExcavationTerrainChunkActor.generated.h"

class UExcavationTerrainChunkSubsystem;
class UExcavationVolumeSubsystem;
class UMaterialInterface;

/**
 * Render/collision adapter for one 3D excavation chunk.
 * All density values remain owned by UExcavationVolumeSubsystem.
 */
UCLASS(BlueprintType)
class MYPROJECT2_API AExcavationTerrainChunkActor : public AActor
{
    GENERATED_BODY()

public:
    AExcavationTerrainChunkActor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Excavation|Terrain")
    bool SetChunkCoordinate(const FIntVector& NewCoordinate);

    UFUNCTION(BlueprintPure, Category = "Excavation|Terrain")
    FIntVector GetChunkCoordinate() const { return ChunkCoordinate; }

    /** Regenerates the visual surface and complex collision from the authoritative density sampler. */
    UFUNCTION(BlueprintCallable, Category = "Excavation|Terrain")
    bool RebuildTerrainChunk();

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Excavation|Terrain")
    FIntVector ChunkCoordinate = FIntVector::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Terrain")
    TObjectPtr<UMaterialInterface> SurfaceMaterial;

    /** A hard ceiling prevents pathological density fields producing an unbounded mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Terrain", meta = (ClampMin = "1000", ClampMax = "1000000"))
    int32 MaxGeneratedTriangles = 200000;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation|Terrain", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UDynamicMeshComponent> DynamicMeshComponent;

protected:
    bool TryGetChunkBaseCell(const UExcavationVolumeSubsystem* Excavation, FIntVector& OutBaseCell) const;
    bool RefreshChunkWorldTransform();
    void ClearMeshAndCollision();

    UPROPERTY(Transient)
    TObjectPtr<UExcavationTerrainChunkSubsystem> RegisteredSubsystem;
};
