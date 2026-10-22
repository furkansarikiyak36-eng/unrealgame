#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralVoxelChunkActor.generated.h"

class UProceduralMeshComponent;

UCLASS()
class MYPROJECT2_API AProceduralVoxelChunkActor : public AActor
{
    GENERATED_BODY()
public:
    AProceduralVoxelChunkActor();

    UFUNCTION(BlueprintCallable, Category = "Voxel|Mesh")
    void SetChunkCoordinate(FIntVector Coordinate) { ChunkCoordinate = Coordinate; }

    UFUNCTION(BlueprintPure, Category = "Voxel|Mesh")
    FIntVector GetChunkCoordinate() const { return ChunkCoordinate; }

    void ApplySurface(const TArray<FVector>& Vertices, const TArray<int32>& Triangles,
        const TArray<FVector>& Normals, bool bCreateCollision, bool bUseAsyncCollision);

    UProceduralMeshComponent* GetVoxelMesh() const { return VoxelMesh; }

protected:
    UPROPERTY(VisibleAnywhere, Category = "Voxel")
    TObjectPtr<UProceduralMeshComponent> VoxelMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
    FIntVector ChunkCoordinate = FIntVector::ZeroValue;
};
