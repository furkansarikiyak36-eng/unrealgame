#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/DynamicMeshComponent.h"
#include "ExcavationTerrainChunkActor.generated.h"

class UExcavationTerrainChunkSubsystem;
class UExcavationVolumeSubsystem;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct MYPROJECT2_API FExcavationTerrainMaterialLayer
{
    GENERATED_BODY()

    /** ID returned by the base density provider. ID 0 uses SurfaceMaterial. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Terrain|Materials", meta = (ClampMin = "1", ClampMax = "31"))
    int32 BaseMaterialId = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Terrain|Materials")
    TObjectPtr<UMaterialInterface> Material;
};

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Terrain|Materials")
    TObjectPtr<UMaterialInterface> SurfaceMaterial;

    /** Optional material slots indexed by stable material IDs from the base density provider. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Terrain|Materials")
    TArray<FExcavationTerrainMaterialLayer> StratifiedMaterialLayers;

    /** A hard ceiling prevents pathological density fields producing an unbounded mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation|Terrain", meta = (ClampMin = "1000", ClampMax = "1000000"))
    int32 MaxGeneratedTriangles = 200000;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Excavation|Terrain", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UDynamicMeshComponent> DynamicMeshComponent;

    /** Cost of the last rebuild in ms: density sampling, meshing, and upload (render mesh, collision, navigation). */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Excavation|Terrain|Stats")
    float LastSampleMs = 0.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Excavation|Terrain|Stats")
    float LastMeshMs = 0.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Excavation|Terrain|Stats")
    float LastUploadMs = 0.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Excavation|Terrain|Stats")
    int32 LastTriangleCount = 0;

protected:
    bool TryGetChunkBaseCell(const UExcavationVolumeSubsystem* Excavation, FIntVector& OutBaseCell) const;
    bool RefreshChunkWorldTransform();
    void ClearMeshAndCollision();

    UPROPERTY(Transient)
    TObjectPtr<UExcavationTerrainChunkSubsystem> RegisteredSubsystem;
};
