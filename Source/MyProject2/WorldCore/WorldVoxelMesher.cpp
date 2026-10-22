// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldVoxelMesher.h"

namespace
{
    const int32 CubeCornerOffsets[8][3] =
    {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
        {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
    };

    // Deterministic six-tetrahedra decomposition of each cube.
    const int32 CubeTetrahedra[6][4] =
    {
        {0, 5, 1, 6},
        {0, 1, 2, 6},
        {0, 2, 3, 6},
        {0, 3, 7, 6},
        {0, 7, 4, 6},
        {0, 4, 5, 6}
    };

    int32 GridIndex(int32 X, int32 Y, int32 Z, int32 Side)
    {
        return (Z * Side + Y) * Side + X;
    }

    struct FSurfaceVertex
    {
        FVector3d Position = FVector3d(0.0, 0.0, 0.0);
        FVector3d Normal = FVector3d(0.0, 0.0, 0.0);
    };

    FSurfaceVertex InterpolateSurfaceVertex(
        int32 NodeA,
        int32 NodeB,
        const TArray<float>& Densities,
        const TArray<FVector3d>& Positions,
        const TArray<FVector3d>& DensityGradients)
    {
        const double DensityA = Densities[NodeA];
        const double DensityB = Densities[NodeB];
        const double Denominator = DensityB - DensityA;
        const double Alpha = FMath::Abs(Denominator) <= SMALL_NUMBER
            ? 0.5
            : FMath::Clamp(
                (static_cast<double>(FWorldMarchingTetrahedraMesher::IsoLevel) - DensityA) / Denominator,
                0.0,
                1.0);

        FSurfaceVertex Result;
        Result.Position = Positions[NodeA] + (Positions[NodeB] - Positions[NodeA]) * Alpha;

        // Density is higher on the solid side. The negative scalar gradient points
        // outward into empty space and supplies smooth normals even across duplicated
        // triangle vertices.
        Result.Normal = -(
            DensityGradients[NodeA]
            + (DensityGradients[NodeB] - DensityGradients[NodeA]) * Alpha).GetSafeNormal();
        Result.Normal = Result.Normal.GetSafeNormal();
        if (Result.Normal.SquaredLength() <= SMALL_NUMBER)
        {
            const bool bSolidA = DensityA >= FWorldMarchingTetrahedraMesher::IsoLevel;
            Result.Normal = bSolidA
                ? (Positions[NodeB] - Positions[NodeA]).GetSafeNormal()
                : (Positions[NodeA] - Positions[NodeB]).GetSafeNormal();
        }
        return Result;
    }

    int32 ResolveSolidMaterial(
        const int32* SolidNodes,
        int32 SolidCount,
        const TArray<int32>& SampleMaterialIds)
    {
        if (SampleMaterialIds.IsEmpty())
        {
            return 0;
        }

        // Prefer an explicit surface-layer ID over the default zero ID. Stable node
        // order gives deterministic results when multiple materials meet one tetrahedron.
        for (int32 Index = 0; Index < SolidCount; ++Index)
        {
            const int32 Node = SolidNodes[Index];
            if (SampleMaterialIds.IsValidIndex(Node) && SampleMaterialIds[Node] > 0)
            {
                return SampleMaterialIds[Node];
            }
        }
        return 0;
    }

    int32 GetOrCreateSurfaceVertexIndex(
        int32 NodeA,
        int32 NodeB,
        const TArray<float>& Densities,
        const TArray<FVector3d>& Positions,
        const TArray<FVector3d>& DensityGradients,
        TMap<uint64, int32>& EdgeVertexCache,
        TMap<int32, int32>& IsoNodeVertexCache,
        FWorldVoxelMeshData& OutMesh)
    {
        const auto GetIsoNodeVertex = [&](int32 Node)
        {
            if (const int32* Existing = IsoNodeVertexCache.Find(Node))
            {
                return *Existing;
            }

            const FVector3d& Position = Positions[Node];
            const FVector3d Normal = -DensityGradients[Node].GetSafeNormal();
            const int32 VertexIndex = OutMesh.Vertices.Num();
            OutMesh.Vertices.Add(FVector(Position.X, Position.Y, Position.Z));
            OutMesh.Normals.Add(FVector(Normal.X, Normal.Y, Normal.Z));
            OutMesh.UV0.Add(FVector2D(Position.X / 100.0, Position.Y / 100.0));
            IsoNodeVertexCache.Add(Node, VertexIndex);
            return VertexIndex;
        };

        if (FMath::IsNearlyEqual(Densities[NodeA], FWorldMarchingTetrahedraMesher::IsoLevel, KINDA_SMALL_NUMBER))
        {
            return GetIsoNodeVertex(NodeA);
        }
        if (FMath::IsNearlyEqual(Densities[NodeB], FWorldMarchingTetrahedraMesher::IsoLevel, KINDA_SMALL_NUMBER))
        {
            return GetIsoNodeVertex(NodeB);
        }

        const uint32 LowNode = static_cast<uint32>(FMath::Min(NodeA, NodeB));
        const uint32 HighNode = static_cast<uint32>(FMath::Max(NodeA, NodeB));
        const uint64 EdgeKey = (static_cast<uint64>(LowNode) << 32) | HighNode;
        if (const int32* Existing = EdgeVertexCache.Find(EdgeKey))
        {
            return *Existing;
        }

        const FSurfaceVertex Vertex = InterpolateSurfaceVertex(
            NodeA, NodeB, Densities, Positions, DensityGradients);
        const int32 VertexIndex = OutMesh.Vertices.Num();
        OutMesh.Vertices.Add(FVector(Vertex.Position.X, Vertex.Position.Y, Vertex.Position.Z));
        OutMesh.Normals.Add(FVector(Vertex.Normal.X, Vertex.Normal.Y, Vertex.Normal.Z));
        OutMesh.UV0.Add(FVector2D(Vertex.Position.X / 100.0, Vertex.Position.Y / 100.0));
        EdgeVertexCache.Add(EdgeKey, VertexIndex);
        return VertexIndex;
    }

    bool AppendTriangle(
        int32 V0,
        int32 V1,
        int32 V2,
        const FVector3d& SolidCentroid,
        const FVector3d& EmptyCentroid,
        int32 MaterialId,
        FWorldVoxelMeshData& OutMesh,
        FString& OutError)
    {
        FVector3d FaceNormal = FVector3d::CrossProduct(
            FVector3d(OutMesh.Vertices[V1]) - FVector3d(OutMesh.Vertices[V0]),
            FVector3d(OutMesh.Vertices[V2]) - FVector3d(OutMesh.Vertices[V0]));
        if (!FMath::IsFinite(FaceNormal.X)
            || !FMath::IsFinite(FaceNormal.Y)
            || !FMath::IsFinite(FaceNormal.Z)
            || FaceNormal.SquaredLength() <= SMALL_NUMBER)
        {
            // Degenerate tetrahedron crossings do not contribute a render/collision triangle.
            return true;
        }

        const FVector3d Outward = EmptyCentroid - SolidCentroid;
        if (FVector3d::DotProduct(FaceNormal, Outward) < 0.0)
        {
            Swap(V1, V2);
        }

        if (OutMesh.TriangleIndices.Num() / 3 >= FWorldMarchingTetrahedraMesher::MaxOutputTriangles)
        {
            OutError = TEXT("Marching Tetrahedra output exceeded the hard triangle budget.");
            return false;
        }

        OutMesh.TriangleIndices.Add(V0);
        OutMesh.TriangleIndices.Add(V1);
        OutMesh.TriangleIndices.Add(V2);
        OutMesh.TriangleMaterialIds.Add(FMath::Max(0, MaterialId));
        return true;
    }

    bool PolygonizeTetrahedron(
        const int32 TetraNodes[4],
        const TArray<float>& Densities,
        const TArray<FVector3d>& Positions,
        const TArray<FVector3d>& DensityGradients,
        const TArray<int32>& SampleMaterialIds,
        TMap<uint64, int32>& EdgeVertexCache,
        TMap<int32, int32>& IsoNodeVertexCache,
        FWorldVoxelMeshData& OutMesh,
        FString& OutError)
    {
        int32 SolidNodes[4];
        int32 EmptyNodes[4];
        int32 SolidCount = 0;
        int32 EmptyCount = 0;
        FVector3d SolidCentroid = FVector3d(0.0, 0.0, 0.0);
        FVector3d EmptyCentroid = FVector3d(0.0, 0.0, 0.0);

        for (int32 I = 0; I < 4; ++I)
        {
            const int32 Node = TetraNodes[I];
            if (Densities[Node] >= FWorldMarchingTetrahedraMesher::IsoLevel)
            {
                SolidNodes[SolidCount++] = Node;
                SolidCentroid += Positions[Node];
            }
            else
            {
                EmptyNodes[EmptyCount++] = Node;
                EmptyCentroid += Positions[Node];
            }
        }

        if (SolidCount == 0 || EmptyCount == 0)
        {
            return true;
        }

        SolidCentroid /= static_cast<double>(SolidCount);
        EmptyCentroid /= static_cast<double>(EmptyCount);
        const int32 MaterialId = ResolveSolidMaterial(SolidNodes, SolidCount, SampleMaterialIds);

        if (SolidCount == 1)
        {
            const int32 A = GetOrCreateSurfaceVertexIndex(SolidNodes[0], EmptyNodes[0], Densities, Positions, DensityGradients, EdgeVertexCache, IsoNodeVertexCache, OutMesh);
            const int32 B = GetOrCreateSurfaceVertexIndex(SolidNodes[0], EmptyNodes[1], Densities, Positions, DensityGradients, EdgeVertexCache, IsoNodeVertexCache, OutMesh);
            const int32 C = GetOrCreateSurfaceVertexIndex(SolidNodes[0], EmptyNodes[2], Densities, Positions, DensityGradients, EdgeVertexCache, IsoNodeVertexCache, OutMesh);
            return AppendTriangle(A, B, C, SolidCentroid, EmptyCentroid, MaterialId, OutMesh, OutError);
        }

        if (SolidCount == 3)
        {
            const int32 A = GetOrCreateSurfaceVertexIndex(EmptyNodes[0], SolidNodes[0], Densities, Positions, DensityGradients, EdgeVertexCache, IsoNodeVertexCache, OutMesh);
            const int32 B = GetOrCreateSurfaceVertexIndex(EmptyNodes[0], SolidNodes[1], Densities, Positions, DensityGradients, EdgeVertexCache, IsoNodeVertexCache, OutMesh);
            const int32 C = GetOrCreateSurfaceVertexIndex(EmptyNodes[0], SolidNodes[2], Densities, Positions, DensityGradients, EdgeVertexCache, IsoNodeVertexCache, OutMesh);
            return AppendTriangle(A, B, C, SolidCentroid, EmptyCentroid, MaterialId, OutMesh, OutError);
        }

        const int32 P00 = GetOrCreateSurfaceVertexIndex(SolidNodes[0], EmptyNodes[0], Densities, Positions, DensityGradients, EdgeVertexCache, IsoNodeVertexCache, OutMesh);
        const int32 P01 = GetOrCreateSurfaceVertexIndex(SolidNodes[0], EmptyNodes[1], Densities, Positions, DensityGradients, EdgeVertexCache, IsoNodeVertexCache, OutMesh);
        const int32 P10 = GetOrCreateSurfaceVertexIndex(SolidNodes[1], EmptyNodes[0], Densities, Positions, DensityGradients, EdgeVertexCache, IsoNodeVertexCache, OutMesh);
        const int32 P11 = GetOrCreateSurfaceVertexIndex(SolidNodes[1], EmptyNodes[1], Densities, Positions, DensityGradients, EdgeVertexCache, IsoNodeVertexCache, OutMesh);
        return AppendTriangle(P00, P01, P11, SolidCentroid, EmptyCentroid, MaterialId, OutMesh, OutError)
            && AppendTriangle(P00, P11, P10, SolidCentroid, EmptyCentroid, MaterialId, OutMesh, OutError);
    }
}

bool FWorldMarchingTetrahedraMesher::BuildMesh(
    const FWorldDensityChunkSnapshot& Input,
    FWorldVoxelMeshData& OutMesh,
    FString& OutError) const
{
    OutMesh = FWorldVoxelMeshData{};
    OutError.Reset();

    if (!Input.IsShapeMetadataValid() || !Input.HasValidSampleStorage())
    {
        OutError = TEXT("Density snapshot metadata or sample storage is invalid.");
        return false;
    }
    if (!Input.MaterialIds.IsEmpty() && Input.MaterialIds.Num() != Input.DensitySamples.Num())
    {
        OutError = TEXT("Material ID sample count must be empty or equal to density sample count.");
        return false;
    }

    const int32 Cells = Input.CellsPerChunkAxis;
    const int32 Border = Input.BorderCells;
    const int32 Side = Cells + 1 + (2 * Border);
    const int32 SampleCount = Input.DensitySamples.Num();
    const double Spacing = Input.SampleSpacingCm;

    TArray<FVector3d> Positions;
    TArray<FVector3d> DensityGradients;
    Positions.SetNumUninitialized(SampleCount);
    DensityGradients.SetNumUninitialized(SampleCount);

    for (int32 Z = 0; Z < Side; ++Z)
    {
        for (int32 Y = 0; Y < Side; ++Y)
        {
            for (int32 X = 0; X < Side; ++X)
            {
                const int32 Index = GridIndex(X, Y, Z, Side);
                const float Density = Input.DensitySamples[Index];
                if (!FMath::IsFinite(Density) || Density < 0.0f || Density > 1.0f)
                {
                    OutError = TEXT("Density samples must be finite and normalized to [0, 1].");
                    OutMesh = FWorldVoxelMeshData{};
                    return false;
                }

                Positions[Index] = FVector3d(
                    (static_cast<double>(X - Border) + 0.5) * Spacing,
                    (static_cast<double>(Y - Border) + 0.5) * Spacing,
                    (static_cast<double>(Z - Border) + 0.5) * Spacing);

                const int32 XM = FMath::Max(0, X - 1);
                const int32 XP = FMath::Min(Side - 1, X + 1);
                const int32 YM = FMath::Max(0, Y - 1);
                const int32 YP = FMath::Min(Side - 1, Y + 1);
                const int32 ZM = FMath::Max(0, Z - 1);
                const int32 ZP = FMath::Min(Side - 1, Z + 1);

                const double DX = (XP - XM) * Spacing;
                const double DY = (YP - YM) * Spacing;
                const double DZ = (ZP - ZM) * Spacing;
                const double GradX = DX > SMALL_NUMBER
                    ? (Input.DensitySamples[GridIndex(XP, Y, Z, Side)] - Input.DensitySamples[GridIndex(XM, Y, Z, Side)]) / DX
                    : 0.0;
                const double GradY = DY > SMALL_NUMBER
                    ? (Input.DensitySamples[GridIndex(X, YP, Z, Side)] - Input.DensitySamples[GridIndex(X, YM, Z, Side)]) / DY
                    : 0.0;
                const double GradZ = DZ > SMALL_NUMBER
                    ? (Input.DensitySamples[GridIndex(X, Y, ZP, Side)] - Input.DensitySamples[GridIndex(X, Y, ZM, Side)]) / DZ
                    : 0.0;
                DensityGradients[Index] = FVector3d(GradX, GradY, GradZ);
            }
        }
    }

    OutMesh.ChunkCoordinate = Input.ChunkCoordinate;
    OutMesh.SourceRevision = Input.SourceRevision;
    TMap<uint64, int32> EdgeVertexCache;
    TMap<int32, int32> IsoNodeVertexCache;
    EdgeVertexCache.Reserve(Cells * Cells * 12);
    IsoNodeVertexCache.Reserve(SampleCount / 16);

    for (int32 Z = 0; Z < Cells; ++Z)
    {
        for (int32 Y = 0; Y < Cells; ++Y)
        {
            for (int32 X = 0; X < Cells; ++X)
            {
                int32 CubeNodes[8];
                for (int32 Corner = 0; Corner < 8; ++Corner)
                {
                    CubeNodes[Corner] = GridIndex(
                        X + Border + CubeCornerOffsets[Corner][0],
                        Y + Border + CubeCornerOffsets[Corner][1],
                        Z + Border + CubeCornerOffsets[Corner][2],
                        Side);
                }

                for (int32 Tetra = 0; Tetra < 6; ++Tetra)
                {
                    int32 TetraNodes[4];
                    for (int32 Corner = 0; Corner < 4; ++Corner)
                    {
                        TetraNodes[Corner] = CubeNodes[CubeTetrahedra[Tetra][Corner]];
                    }

                    if (!PolygonizeTetrahedron(
                        TetraNodes,
                        Input.DensitySamples,
                        Positions,
                        DensityGradients,
                        Input.MaterialIds,
                        EdgeVertexCache,
                        IsoNodeVertexCache,
                        OutMesh,
                        OutError))
                    {
                        OutMesh = FWorldVoxelMeshData{};
                        return false;
                    }
                }
            }
        }
    }

    if (!OutMesh.IsTopologyValid())
    {
        OutError = TEXT("Marching Tetrahedra generated invalid mesh topology or attribute buffers.");
        OutMesh = FWorldVoxelMeshData{};
        return false;
    }
    return true;
}
