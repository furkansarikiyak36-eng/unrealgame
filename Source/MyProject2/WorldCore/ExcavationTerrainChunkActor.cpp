#include "ExcavationTerrainChunkActor.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "ExcavationTerrainChunkSubsystem.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "HAL/PlatformTime.h"

namespace
{
    constexpr float SurfaceIsoLevel = 0.5f;
    constexpr int32 MaxSupportedChunkCellsPerAxis = 64;

    // Cube corners, ordered consistently across adjacent cubes so shared faces use matching diagonals.
    const int32 CubeCornerOffsets[8][3] =
    {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
        {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
    };

    // Six tetrahedra share the 0-to-6 cube diagonal (marching tetrahedra).
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

    FVector3d InterpolateIsoSurface(
        int32 A,
        int32 B,
        const TArray<float>& Densities,
        const TArray<FVector3d>& Positions)
    {
        const double DensityA = Densities[A];
        const double DensityB = Densities[B];
        const double Denominator = DensityB - DensityA;
        if (FMath::Abs(Denominator) <= SMALL_NUMBER)
        {
            return (Positions[A] + Positions[B]) * 0.5;
        }

        const double Alpha = FMath::Clamp((static_cast<double>(SurfaceIsoLevel) - DensityA) / Denominator, 0.0, 1.0);
        return Positions[A] + (Positions[B] - Positions[A]) * Alpha;
    }

    void AddOrientedTriangle(
        UE::Geometry::FDynamicMesh3& Mesh,
        const FVector3d& A,
        const FVector3d& B,
        const FVector3d& C,
        const FVector3d& SolidCentroid,
        const FVector3d& EmptyCentroid,
        int32 MaterialSlotId,
        int32 TriangleLimit,
        bool& bExceededTriangleLimit)
    {
        if (bExceededTriangleLimit)
        {
            return;
        }

        if (Mesh.TriangleCount() >= TriangleLimit)
        {
            bExceededTriangleLimit = true;
            return;
        }

        FVector3d P0 = A;
        FVector3d P1 = B;
        FVector3d P2 = C;
        // FDynamicMesh3 front faces (render culling, normals, one-sided physics) follow GeometryCore's
        // VectorUtil::Normal = (C - A) x (B - A); wind so that points out of the solid.
        const FVector3d Normal = FVector3d::CrossProduct(P2 - P0, P1 - P0);
        const FVector3d Outward = EmptyCentroid - SolidCentroid;
        if (FVector3d::DotProduct(Normal, Outward) < 0.0)
        {
            Swap(P1, P2);
        }

        const int32 V0 = Mesh.AppendVertex(P0);
        const int32 V1 = Mesh.AppendVertex(P1);
        const int32 V2 = Mesh.AppendVertex(P2);
        if (V0 == UE::Geometry::FDynamicMesh3::InvalidID
            || V1 == UE::Geometry::FDynamicMesh3::InvalidID
            || V2 == UE::Geometry::FDynamicMesh3::InvalidID)
        {
            bExceededTriangleLimit = true;
            return;
        }

        const int32 TriangleId = Mesh.AppendTriangle(V0, V1, V2, 0);
        if (TriangleId < 0)
        {
            // Vertices are unique per triangle; a rejected primitive indicates the candidate is unusable.
            bExceededTriangleLimit = true;
            return;
        }

        if (Mesh.Attributes() && Mesh.Attributes()->HasMaterialID())
        {
            Mesh.Attributes()->GetMaterialID()->SetValue(TriangleId, MaterialSlotId);
        }
    }

    bool PolygonizeTetrahedron(
        UE::Geometry::FDynamicMesh3& Mesh,
        const int32 TetraNodeIds[4],
        const TArray<float>& Densities,
        const TArray<FVector3d>& Positions,
        int32 MaterialSlotId,
        int32 TriangleLimit,
        bool& bExceededTriangleLimit)
    {
        int32 SolidNodes[4];
        int32 EmptyNodes[4];
        int32 SolidCount = 0;
        int32 EmptyCount = 0;
        FVector3d SolidCentroid(0.0, 0.0, 0.0);
        FVector3d EmptyCentroid(0.0, 0.0, 0.0);

        for (int32 I = 0; I < 4; ++I)
        {
            const int32 Node = TetraNodeIds[I];
            if (Densities[Node] >= SurfaceIsoLevel)
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

        if (SolidCount == 1)
        {
            const FVector3d A = InterpolateIsoSurface(SolidNodes[0], EmptyNodes[0], Densities, Positions);
            const FVector3d B = InterpolateIsoSurface(SolidNodes[0], EmptyNodes[1], Densities, Positions);
            const FVector3d C = InterpolateIsoSurface(SolidNodes[0], EmptyNodes[2], Densities, Positions);
            AddOrientedTriangle(Mesh, A, B, C, SolidCentroid, EmptyCentroid, MaterialSlotId, TriangleLimit, bExceededTriangleLimit);
        }
        else if (SolidCount == 3)
        {
            const FVector3d A = InterpolateIsoSurface(EmptyNodes[0], SolidNodes[0], Densities, Positions);
            const FVector3d B = InterpolateIsoSurface(EmptyNodes[0], SolidNodes[1], Densities, Positions);
            const FVector3d C = InterpolateIsoSurface(EmptyNodes[0], SolidNodes[2], Densities, Positions);
            AddOrientedTriangle(Mesh, A, B, C, SolidCentroid, EmptyCentroid, MaterialSlotId, TriangleLimit, bExceededTriangleLimit);
        }
        else
        {
            const FVector3d P00 = InterpolateIsoSurface(SolidNodes[0], EmptyNodes[0], Densities, Positions);
            const FVector3d P01 = InterpolateIsoSurface(SolidNodes[0], EmptyNodes[1], Densities, Positions);
            const FVector3d P10 = InterpolateIsoSurface(SolidNodes[1], EmptyNodes[0], Densities, Positions);
            const FVector3d P11 = InterpolateIsoSurface(SolidNodes[1], EmptyNodes[1], Densities, Positions);
            AddOrientedTriangle(Mesh, P00, P01, P11, SolidCentroid, EmptyCentroid, MaterialSlotId, TriangleLimit, bExceededTriangleLimit);
            AddOrientedTriangle(Mesh, P00, P11, P10, SolidCentroid, EmptyCentroid, MaterialSlotId, TriangleLimit, bExceededTriangleLimit);
        }

        return !bExceededTriangleLimit;
    }
}

AExcavationTerrainChunkActor::AExcavationTerrainChunkActor()
{
    PrimaryActorTick.bCanEverTick = false;

    DynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("ExcavationDynamicMesh"));
    SetRootComponent(DynamicMeshComponent);

    DynamicMeshComponent->SetCollisionObjectType(ECC_WorldStatic);
    DynamicMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
    DynamicMeshComponent->SetComplexAsSimpleCollisionEnabled(true, false);
    DynamicMeshComponent->bUseAsyncCooking = true;
    DynamicMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DynamicMeshComponent->SetCanEverAffectNavigation(true);
    DynamicMeshComponent->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
}

void AExcavationTerrainChunkActor::BeginPlay()
{
    Super::BeginPlay();

    // Project defaults when nothing is set: soil on top, rock (base material 2) where the walls reach bedrock or a boulder.
    if (!SurfaceMaterial)
    {
        SurfaceMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Excavation/MI_VoxelSoil.MI_VoxelSoil"));
    }
    if (StratifiedMaterialLayers.IsEmpty())
    {
        if (UMaterialInterface* Rock = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Excavation/MI_VoxelRock.MI_VoxelRock")))
        {
            FExcavationTerrainMaterialLayer Layer;
            Layer.BaseMaterialId = 2;
            Layer.Material = Rock;
            StratifiedMaterialLayers.Add(Layer);
        }
    }

    if (RefreshChunkWorldTransform())
    {
        if (UWorld* World = GetWorld())
        {
            RegisteredSubsystem = World->GetSubsystem<UExcavationTerrainChunkSubsystem>();
            if (RegisteredSubsystem)
            {
                RegisteredSubsystem->RegisterChunkActor(this);
            }
        }
        RebuildTerrainChunk();
    }
}

void AExcavationTerrainChunkActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(RegisteredSubsystem))
    {
        RegisteredSubsystem->UnregisterChunkActor(this);
    }
    RegisteredSubsystem = nullptr;
    Super::EndPlay(EndPlayReason);
}

bool AExcavationTerrainChunkActor::SetChunkCoordinate(const FIntVector& NewCoordinate)
{
    if (NewCoordinate == ChunkCoordinate)
    {
        return true;
    }

    const FIntVector PreviousCoordinate = ChunkCoordinate;
    ChunkCoordinate = NewCoordinate;

    if (HasActorBegunPlay())
    {
        if (!RefreshChunkWorldTransform())
        {
            ChunkCoordinate = PreviousCoordinate;
            RefreshChunkWorldTransform();
            return false;
        }

        if (IsValid(RegisteredSubsystem))
        {
            RegisteredSubsystem->UnregisterChunkActor(this);
            RegisteredSubsystem->RegisterChunkActor(this);
        }
        return RebuildTerrainChunk();
    }

    return true;
}

bool AExcavationTerrainChunkActor::TryGetChunkBaseCell(
    const UExcavationVolumeSubsystem* Excavation,
    FIntVector& OutBaseCell) const
{
    if (!IsValid(Excavation)
        || !FMath::IsFinite(Excavation->CellSizeCm)
        || Excavation->CellSizeCm < 1.0f
        || Excavation->CellsPerChunkAxis < 4
        || Excavation->CellsPerChunkAxis > MaxSupportedChunkCellsPerAxis)
    {
        return false;
    }

    const int64 Cells = Excavation->CellsPerChunkAxis;
    const int64 BaseX = static_cast<int64>(ChunkCoordinate.X) * Cells;
    const int64 BaseY = static_cast<int64>(ChunkCoordinate.Y) * Cells;
    const int64 BaseZ = static_cast<int64>(ChunkCoordinate.Z) * Cells;
    const int64 MinCell = TNumericLimits<int32>::Lowest();
    const int64 MaxCell = TNumericLimits<int32>::Max();

    // The mesh samples one extra border cell on every axis; avoid int32 overflow at world extremes.
    if (BaseX < MinCell || BaseY < MinCell || BaseZ < MinCell
        || BaseX + Cells > MaxCell || BaseY + Cells > MaxCell || BaseZ + Cells > MaxCell)
    {
        return false;
    }

    OutBaseCell = FIntVector(
        static_cast<int32>(BaseX),
        static_cast<int32>(BaseY),
        static_cast<int32>(BaseZ));
    return true;
}

bool AExcavationTerrainChunkActor::RefreshChunkWorldTransform()
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;

    FIntVector BaseCell;
    if (!TryGetChunkBaseCell(Excavation, BaseCell))
    {
        return false;
    }

    const double CellSize = Excavation->CellSizeCm;
    SetActorLocation(FVector(
        static_cast<double>(BaseCell.X) * CellSize,
        static_cast<double>(BaseCell.Y) * CellSize,
        static_cast<double>(BaseCell.Z) * CellSize));
    SetActorRotation(FRotator::ZeroRotator);
    SetActorScale3D(FVector::OneVector);
    return true;
}

void AExcavationTerrainChunkActor::ClearMeshAndCollision()
{
    if (!DynamicMeshComponent)
    {
        return;
    }

    UE::Geometry::FDynamicMesh3 EmptyMesh;
    DynamicMeshComponent->SetMesh(MoveTemp(EmptyMesh));
    DynamicMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DynamicMeshComponent->UpdateCollision(false);
    UNavigationSystemV1::UpdateComponentInNavOctree(*DynamicMeshComponent);
}

bool AExcavationTerrainChunkActor::RebuildTerrainChunk()
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;

    FIntVector BaseCell;
    if (!DynamicMeshComponent || !RefreshChunkWorldTransform()
        || !TryGetChunkBaseCell(Excavation, BaseCell))
    {
        ClearMeshAndCollision();
        return false;
    }

    DynamicMeshComponent->SetMaterial(0, SurfaceMaterial ? SurfaceMaterial.Get() : UMaterial::GetDefaultMaterial(MD_Surface));

    TMap<int32, int32> MaterialSlotByBaseId;
    MaterialSlotByBaseId.Add(0, 0);
    int32 NextMaterialSlot = 1;
    for (const FExcavationTerrainMaterialLayer& Layer : StratifiedMaterialLayers)
    {
        if (Layer.BaseMaterialId <= 0 || Layer.BaseMaterialId > 31
            || !IsValid(Layer.Material) || MaterialSlotByBaseId.Contains(Layer.BaseMaterialId))
        {
            continue;
        }

        MaterialSlotByBaseId.Add(Layer.BaseMaterialId, NextMaterialSlot);
        DynamicMeshComponent->SetMaterial(NextMaterialSlot, Layer.Material);
        ++NextMaterialSlot;
    }

    const int32 Cells = Excavation->CellsPerChunkAxis;
    const int32 Side = Cells + 1;
    const int32 SampleCount = Side * Side * Side;
    const double CellSize = Excavation->CellSizeCm;
    const double HalfCell = CellSize * 0.5;

    const double TimeStart = FPlatformTime::Seconds();
    TArray<float> Densities;
    TArray<FVector3d> Positions;
    Densities.SetNumUninitialized(SampleCount);
    Positions.SetNumUninitialized(SampleCount);

    for (int32 Z = 0; Z < Side; ++Z)
    {
        for (int32 Y = 0; Y < Side; ++Y)
        {
            for (int32 X = 0; X < Side; ++X)
            {
                Positions[GridIndex(X, Y, Z, Side)] = FVector3d(
                    (static_cast<double>(X) * CellSize) + HalfCell,
                    (static_cast<double>(Y) * CellSize) + HalfCell,
                    (static_cast<double>(Z) * CellSize) + HalfCell);
            }
        }
    }
    if (!Excavation->SampleDensityBlock(BaseCell, Side, Densities))
    {
        ClearMeshAndCollision();
        return false;
    }

    const double TimeSampled = FPlatformTime::Seconds();
    LastSampleMs = static_cast<float>((TimeSampled - TimeStart) * 1000.0);

    // All solid or all empty: no surface. Skip meshing, and the upload too when the chunk is already empty.
    bool bAnySolid = false;
    bool bAnyEmpty = false;
    for (const float Density : Densities)
    {
        (Density >= SurfaceIsoLevel ? bAnySolid : bAnyEmpty) = true;
        if (bAnySolid && bAnyEmpty)
        {
            break;
        }
    }
    if (!bAnySolid || !bAnyEmpty)
    {
        LastMeshMs = 0.0f;
        const bool bHadMesh = DynamicMeshComponent->GetMesh() && DynamicMeshComponent->GetMesh()->TriangleCount() > 0;
        if (bHadMesh)
        {
            ClearMeshAndCollision();
        }
        LastUploadMs = static_cast<float>((FPlatformTime::Seconds() - TimeSampled) * 1000.0);
        LastTriangleCount = 0;
        return true;
    }

    UE::Geometry::FDynamicMesh3 CandidateMesh;
    CandidateMesh.EnableAttributes();
    CandidateMesh.Attributes()->EnableMaterialID();
    bool bExceededTriangleLimit = false;
    const FVector3d ActorOrigin(GetActorLocation());

    for (int32 Z = 0; Z < Cells && !bExceededTriangleLimit; ++Z)
    {
        for (int32 Y = 0; Y < Cells && !bExceededTriangleLimit; ++Y)
        {
            for (int32 X = 0; X < Cells && !bExceededTriangleLimit; ++X)
            {
                int32 CubeNodes[8];
                for (int32 Corner = 0; Corner < 8; ++Corner)
                {
                    CubeNodes[Corner] = GridIndex(
                        X + CubeCornerOffsets[Corner][0],
                        Y + CubeCornerOffsets[Corner][1],
                        Z + CubeCornerOffsets[Corner][2],
                        Side);
                }

                // Most cubes are wholly solid or wholly empty; only the ones the surface crosses make triangles.
                int32 SolidCorners = 0;
                for (int32 Corner = 0; Corner < 8; ++Corner)
                {
                    SolidCorners += Densities[CubeNodes[Corner]] >= SurfaceIsoLevel ? 1 : 0;
                }
                if (SolidCorners == 0 || SolidCorners == 8)
                {
                    continue;
                }

                // Material of the solid side, sampled once per surface cube.
                int32 MaterialSlotId = 0;
                if (MaterialSlotByBaseId.Num() > 1)
                {
                    FVector3d SolidCentroid(0.0, 0.0, 0.0);
                    for (int32 Corner = 0; Corner < 8; ++Corner)
                    {
                        if (Densities[CubeNodes[Corner]] >= SurfaceIsoLevel) SolidCentroid += Positions[CubeNodes[Corner]];
                    }
                    SolidCentroid /= static_cast<double>(SolidCorners);
                    int32 BaseMaterialId = 0;
                    if (Excavation->TrySampleBaseMaterialIdAtWorld(FVector(ActorOrigin + SolidCentroid), BaseMaterialId))
                    {
                        if (const int32* ResolvedSlot = MaterialSlotByBaseId.Find(BaseMaterialId))
                        {
                            MaterialSlotId = *ResolvedSlot;
                        }
                    }
                }

                for (int32 Tetra = 0; Tetra < 6 && !bExceededTriangleLimit; ++Tetra)
                {
                    int32 TetraNodes[4];
                    for (int32 Corner = 0; Corner < 4; ++Corner)
                    {
                        TetraNodes[Corner] = CubeNodes[CubeTetrahedra[Tetra][Corner]];
                    }

                    PolygonizeTetrahedron(
                        CandidateMesh,
                        TetraNodes,
                        Densities,
                        Positions,
                        MaterialSlotId,
                        FMath::Max(1000, MaxGeneratedTriangles),
                        bExceededTriangleLimit);
                }
            }
        }
    }

    if (bExceededTriangleLimit)
    {
        ClearMeshAndCollision();
        return false;
    }

    // Face normals into the normal overlay (the renderer reads the overlay; vertices are not shared, so shading is flat).
    if (UE::Geometry::FDynamicMeshNormalOverlay* Normals = CandidateMesh.Attributes() ? CandidateMesh.Attributes()->PrimaryNormals() : nullptr)
    {
        for (const int32 TriangleId : CandidateMesh.TriangleIndicesItr())
        {
            const int32 Element = Normals->AppendElement(static_cast<FVector3f>(CandidateMesh.GetTriNormal(TriangleId)));
            Normals->SetTriangle(TriangleId, UE::Geometry::FIndex3i(Element, Element, Element));
        }
    }

    const double TimeMeshed = FPlatformTime::Seconds();
    LastMeshMs = static_cast<float>((TimeMeshed - TimeSampled) * 1000.0);
    LastTriangleCount = CandidateMesh.TriangleCount();

    DynamicMeshComponent->SetMesh(MoveTemp(CandidateMesh));
    if (DynamicMeshComponent->GetMesh() && DynamicMeshComponent->GetMesh()->TriangleCount() > 0)
    {
        DynamicMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        DynamicMeshComponent->UpdateCollision(false);
    }
    else
    {
        DynamicMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        DynamicMeshComponent->UpdateCollision(false);
    }

    UNavigationSystemV1::UpdateComponentInNavOctree(*DynamicMeshComponent);
    LastUploadMs = static_cast<float>((FPlatformTime::Seconds() - TimeMeshed) * 1000.0);
    return true;
}
