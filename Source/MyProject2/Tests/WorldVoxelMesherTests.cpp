// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "WorldCore/WorldVoxelMesher.h"
#include "Misc/AutomationTest.h"

namespace
{
    FWorldDensityChunkSnapshot MakeHorizontalPlaneSnapshot()
    {
        FWorldDensityChunkSnapshot Snapshot;
        Snapshot.ChunkCoordinate = FIntVector::ZeroValue;
        Snapshot.CellsPerChunkAxis = 2;
        Snapshot.BorderCells = 1;
        Snapshot.SampleSpacingCm = 25.0f;
        Snapshot.SourceRevision = 17;
        const int32 Side = Snapshot.CellsPerChunkAxis + 1 + 2 * Snapshot.BorderCells;
        Snapshot.DensitySamples.SetNumUninitialized(Side * Side * Side);

        for (int32 Z = 0; Z < Side; ++Z)
        {
            for (int32 Y = 0; Y < Side; ++Y)
            {
                for (int32 X = 0; X < Side; ++X)
                {
                    const int32 Index = (Z * Side + Y) * Side + X;
                    // ISO=0.5 should interpolate halfway between sample planes at Z=37.5 and Z=62.5.
                    Snapshot.DensitySamples[Index] = Z <= 2 ? 1.0f : 0.0f;
                }
            }
        }
        return Snapshot;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldVoxelMesherHorizontalPlaneTest,
    "MyProject2.World.VoxelMesher.HorizontalPlane",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldVoxelMesherHorizontalPlaneTest::RunTest(const FString& Parameters)
{
    FWorldDensityChunkSnapshot Snapshot = MakeHorizontalPlaneSnapshot();
    FWorldMarchingTetrahedraMesher Mesher;
    FWorldVoxelMeshData Mesh;
    FString Error;

    TestTrue(TEXT("Valid density snapshot should produce a smooth mesh"), Mesher.BuildMesh(Snapshot, Mesh, Error));
    TestTrue(TEXT("Successful meshing should not leave an error"), Error.IsEmpty());
    TestTrue(TEXT("Generated topology should validate"), Mesh.IsTopologyValid());
    TestTrue(TEXT("Horizontal plane should generate triangles"), Mesh.TriangleIndices.Num() >= 3);
    TestEqual(TEXT("Source revision should be retained"), Mesh.SourceRevision, Snapshot.SourceRevision);

    if (Mesh.Vertices.IsEmpty())
    {
        AddError(TEXT("Mesher returned no vertices for a density field with a crossing."));
        return false;
    }

    int32 UpwardNormals = 0;
    for (int32 Index = 0; Index < Mesh.Vertices.Num(); ++Index)
    {
        TestTrue(
            TEXT("Interpolated plane vertex should lie at the ISO height"),
            FMath::Abs(Mesh.Vertices[Index].Z - 50.0f) < 0.1f);
        if (Mesh.Normals[Index].Z > 0.9f)
        {
            ++UpwardNormals;
        }
    }

    TestEqual(
        TEXT("All plane normals should point upward into air"),
        UpwardNormals,
        Mesh.Normals.Num());
    TestEqual(
        TEXT("One material ID should be emitted per triangle"),
        Mesh.TriangleMaterialIds.Num(),
        Mesh.TriangleIndices.Num() / 3);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldVoxelMesherInvalidDensityTest,
    "MyProject2.World.VoxelMesher.InvalidSampleRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldVoxelMesherInvalidDensityTest::RunTest(const FString& Parameters)
{
    FWorldDensityChunkSnapshot Snapshot = MakeHorizontalPlaneSnapshot();
    FWorldMarchingTetrahedraMesher Mesher;
    FWorldVoxelMeshData Mesh;
    FString Error;

    Snapshot.DensitySamples[0] = 1.25f;
    TestFalse(TEXT("Density values above 1.0 must be rejected"), Mesher.BuildMesh(Snapshot, Mesh, Error));
    TestFalse(TEXT("Rejected density should include an error reason"), Error.IsEmpty());

    Snapshot = MakeHorizontalPlaneSnapshot();
    Snapshot.MaterialIds.Add(1);
    Error.Reset();
    TestFalse(TEXT("A truncated material sample buffer must be rejected"), Mesher.BuildMesh(Snapshot, Mesh, Error));
    TestFalse(TEXT("Invalid material buffer should include an error reason"), Error.IsEmpty());

    Snapshot = MakeHorizontalPlaneSnapshot();
    Snapshot.DensitySamples.SetNum(7);
    Error.Reset();
    TestFalse(TEXT("A truncated density sample buffer must be rejected"), Mesher.BuildMesh(Snapshot, Mesh, Error));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
