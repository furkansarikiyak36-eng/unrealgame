// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldRuntimeProfile.h"

bool FWorldRuntimeProfileValues::HasConsistentChunkGrid(float ToleranceCm) const
{
    if (!FMath::IsFinite(VoxelSampleSpacingCm)
        || !FMath::IsFinite(ChunkSizeCm)
        || !FMath::IsFinite(ToleranceCm)
        || ToleranceCm < 0.0f
        || VoxelSampleSpacingCm <= 0.0f
        || ChunkSizeCm <= 0.0f
        || CellsPerChunkAxis <= 0)
    {
        return false;
    }

    const float DerivedChunkSize = VoxelSampleSpacingCm * static_cast<float>(CellsPerChunkAxis);
    return FMath::IsFinite(DerivedChunkSize)
        && FMath::IsNearlyEqual(DerivedChunkSize, ChunkSizeCm, ToleranceCm);
}

bool FWorldRuntimeProfileValues::Validate(FString* OutError) const
{
    if (OutError)
    {
        OutError->Reset();
    }

    const auto Fail = [OutError](const TCHAR* Message)
    {
        if (OutError)
        {
            *OutError = Message;
        }
        return false;
    };

    if (!FMath::IsFinite(VoxelSampleSpacingCm) || VoxelSampleSpacingCm < 1.0f)
    {
        return Fail(TEXT("VoxelSampleSpacingCm must be finite and at least 1 cm."));
    }
    if (!FMath::IsFinite(ChunkSizeCm) || ChunkSizeCm < 100.0f)
    {
        return Fail(TEXT("ChunkSizeCm must be finite and at least 100 cm."));
    }
    if (CellsPerChunkAxis < 4 || CellsPerChunkAxis > 512)
    {
        return Fail(TEXT("CellsPerChunkAxis must be in [4, 512]."));
    }
    if (!HasConsistentChunkGrid())
    {
        return Fail(TEXT("ChunkSizeCm must match VoxelSampleSpacingCm * CellsPerChunkAxis within 1 cm."));
    }
    if (MaxMeshJobsInFlight < 1 || MaxMeshJobsInFlight > 64)
    {
        return Fail(TEXT("MaxMeshJobsInFlight must be in [1, 64]."));
    }
    if (MaxQueuedMeshJobs < 1 || MaxQueuedMeshJobs > 4096)
    {
        return Fail(TEXT("MaxQueuedMeshJobs must be in [1, 4096]."));
    }
    if (MaxChunksPublishedPerFrame < 1 || MaxChunksPublishedPerFrame > 64)
    {
        return Fail(TEXT("MaxChunksPublishedPerFrame must be in [1, 64]."));
    }
    if (MaxCollisionUpdatesPerFrame < 1 || MaxCollisionUpdatesPerFrame > 64)
    {
        return Fail(TEXT("MaxCollisionUpdatesPerFrame must be in [1, 64]."));
    }
    if (MaxSurfaceSamplesPerAxis < 1 || MaxSurfaceSamplesPerAxis > 32)
    {
        return Fail(TEXT("MaxSurfaceSamplesPerAxis must be in [1, 32]."));
    }
    if (MaxActiveEditableChunks < 1 || MaxActiveEditableChunks > 100000)
    {
        return Fail(TEXT("MaxActiveEditableChunks must be in [1, 100000]."));
    }
    if (MaxDensitySamplesPerEdit < 1 || MaxDensitySamplesPerEdit > 50000000)
    {
        return Fail(TEXT("MaxDensitySamplesPerEdit must be in [1, 50000000]."));
    }

    if (OutError)
    {
        OutError->Reset();
    }
    return true;
}

UWorldRuntimeProfileDataAsset::UWorldRuntimeProfileDataAsset()
{
    LowMid.VoxelSampleSpacingCm = 25.0f;
    LowMid.ChunkSizeCm = 800.0f;
    LowMid.CellsPerChunkAxis = 32;
    LowMid.MaxMeshJobsInFlight = 1;
    LowMid.MaxQueuedMeshJobs = 8;
    LowMid.MaxChunksPublishedPerFrame = 1;
    LowMid.MaxCollisionUpdatesPerFrame = 1;
    LowMid.MaxSurfaceSamplesPerAxis = 5;
    LowMid.MaxActiveEditableChunks = 32;
    LowMid.MaxDensitySamplesPerEdit = 100000;
    LowMid.bUseAdaptiveVoxelLOD = false;
    LowMid.bAllowBackgroundMeshing = true;
    LowMid.bAllowAsyncCollision = false;

    HighEnd.VoxelSampleSpacingCm = 12.5f;
    HighEnd.ChunkSizeCm = 800.0f;
    HighEnd.CellsPerChunkAxis = 64;
    HighEnd.MaxMeshJobsInFlight = 3;
    HighEnd.MaxQueuedMeshJobs = 32;
    HighEnd.MaxChunksPublishedPerFrame = 2;
    HighEnd.MaxCollisionUpdatesPerFrame = 2;
    HighEnd.MaxSurfaceSamplesPerAxis = 9;
    HighEnd.MaxActiveEditableChunks = 96;
    HighEnd.MaxDensitySamplesPerEdit = 500000;
    HighEnd.bUseAdaptiveVoxelLOD = true;
    HighEnd.bAllowBackgroundMeshing = true;
    HighEnd.bAllowAsyncCollision = true;
}
