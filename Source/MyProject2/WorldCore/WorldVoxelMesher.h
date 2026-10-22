// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorldCore/WorldSystemContracts.h"

/**
 * CPU-only Marching Tetrahedra extractor.
 * Output vertex positions are chunk-local centimetres; density remains owned by
 * UExcavationVolumeSubsystem and is supplied as an immutable snapshot.
 */
class MYPROJECT2_API FWorldMarchingTetrahedraMesher final : public IVoxelSurfaceMesher
{
public:
    virtual bool BuildMesh(
        const FWorldDensityChunkSnapshot& Input,
        FWorldVoxelMeshData& OutMesh,
        FString& OutError) const override;

    static constexpr float IsoLevel = 0.5f;
    static constexpr int32 MaxOutputTriangles = 1000000;
};
