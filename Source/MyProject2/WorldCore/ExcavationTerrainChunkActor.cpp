#include "ExcavationTerrainChunkActor.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "WorldCore/WorldVoxelMesher.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "ExcavationTerrainChunkSubsystem.h"
#include "Engine/World.h"
#include "NavigationSystem.h"

namespace
{
    constexpr int32 MaxSupportedChunkCellsPerAxis = 64;

    bool ConvertMeshDataToDynamicMesh(
        const FWorldVoxelMeshData& Source,
        const TMap<int32, int32>& MaterialSlotByBaseId,
        UE::Geometry::FDynamicMesh3& OutMesh,
        FString& OutError)
    {
        OutError.Reset();
        OutMesh = UE::Geometry::FDynamicMesh3();
        if (!Source.IsTopologyValid())
        {
            OutError = TEXT("CPU voxel mesh data failed topology validation.");
            return false;
        }

        OutMesh.EnableAttributes();
        OutMesh.Attributes()->EnableMaterialID();
        UE::Geometry::FDynamicMeshNormalOverlay* NormalOverlay = OutMesh.Attributes()->PrimaryNormals();
        UE::Geometry::FDynamicMeshUVOverlay* UVOverlay = OutMesh.Attributes()->PrimaryUV();
        if (!NormalOverlay || !UVOverlay || !OutMesh.Attributes()->GetMaterialID())
        {
            OutError = TEXT("Dynamic mesh attributes could not be initialized.");
            return false;
        }

        TArray<int32> VertexIds;
        TArray<int32> NormalIds;
        TArray<int32> UVIds;
        VertexIds.Reserve(Source.Vertices.Num());
        NormalIds.Reserve(Source.Vertices.Num());
        UVIds.Reserve(Source.Vertices.Num());

        for (int32 Index = 0; Index < Source.Vertices.Num(); ++Index)
        {
            const FVector& Position = Source.Vertices[Index];
            const FVector& Normal = Source.Normals[Index];
            const FVector2D& UV = Source.UV0.IsEmpty() ? FVector2D::ZeroVector : Source.UV0[Index];
            const int32 VertexId = OutMesh.AppendVertex(FVector3d(Position));
            if (VertexId == UE::Geometry::FDynamicMesh3::InvalidID)
            {
                OutError = TEXT("Dynamic mesh rejected a generated vertex.");
                return false;
            }

            const int32 NormalId = NormalOverlay->AppendElement(FVector3f(Normal), VertexId);
            const int32 UVId = UVOverlay->AppendElement(FVector2f(UV), VertexId);
            if (NormalId < 0 || UVId < 0)
            {
                OutError = TEXT("Dynamic mesh rejected a normal or UV overlay element.");
                return false;
            }

            VertexIds.Add(VertexId);
            NormalIds.Add(NormalId);
            UVIds.Add(UVId);
        }

        for (int32 Triangle = 0; Triangle < Source.TriangleIndices.Num() / 3; ++Triangle)
        {
            const int32 SourceIndex = Triangle * 3;
            const int32 SourceV0 = Source.TriangleIndices[SourceIndex];
            const int32 SourceV1 = Source.TriangleIndices[SourceIndex + 1];
            const int32 SourceV2 = Source.TriangleIndices[SourceIndex + 2];
            const int32 TriangleId = OutMesh.AppendTriangle(
                VertexIds[SourceV0], VertexIds[SourceV1], VertexIds[SourceV2], 0);
            if (TriangleId < 0)
            {
                OutError = TEXT("Dynamic mesh rejected a generated triangle.");
                return false;
            }

            NormalOverlay->SetTriangle(
                TriangleId,
                FIndex3i(NormalIds[SourceV0], NormalIds[SourceV1], NormalIds[SourceV2]),
                false);
            UVOverlay->SetTriangle(
                TriangleId,
                FIndex3i(UVIds[SourceV0], UVIds[SourceV1], UVIds[SourceV2]),
                false);

            const int32 BaseMaterialId = Source.TriangleMaterialIds.IsEmpty()
                ? 0
                : Source.TriangleMaterialIds[Triangle];
            const int32* MaterialSlot = MaterialSlotByBaseId.Find(BaseMaterialId);
            OutMesh.Attributes()->GetMaterialID()->SetValue(
                TriangleId, MaterialSlot ? *MaterialSlot : 0);
        }

        if (!OutMesh.CheckValidity(UE::Geometry::FDynamicMesh3::FValidityOptions(false, false), UE::Geometry::EValidityCheckFailMode::ReturnOnly))
        {
            OutError = TEXT("Converted dynamic mesh failed GeometryCore structural validation.");
            return false;
        }
        return true;
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
    DynamicMeshComponent->SetDeferredCollisionUpdatesEnabled(true, false);
    DynamicMeshComponent->bUseAsyncCooking = false;
    Tags.AddUnique(FName(TEXT("World.Object.VoxelSurface")));
    DynamicMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DynamicMeshComponent->SetCanEverAffectNavigation(true);
    DynamicMeshComponent->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
}

void AExcavationTerrainChunkActor::BeginPlay()
{
    Super::BeginPlay();

    if (RefreshChunkWorldTransform())
    {
        if (UWorld* World = GetWorld())
        {
            RegisteredSubsystem = World->GetSubsystem<UExcavationTerrainChunkSubsystem>();
            if (RegisteredSubsystem)
            {
                RegisteredSubsystem->RegisterChunkActor(this);
                RegisteredSubsystem->QueueChunkRebuild(ChunkCoordinate);
                return;
            }
        }
        // Fallback for nonstandard worlds that do not create the renderer subsystem.
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
            RegisteredSubsystem->QueueChunkRebuild(ChunkCoordinate);
            return true;
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

void AExcavationTerrainChunkActor::InvalidateVoxelMesh()
{
    ClearMeshAndCollision();
}

bool AExcavationTerrainChunkActor::RebuildTerrainChunk()
{
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!DynamicMeshComponent || !IsValid(Excavation) || !RefreshChunkWorldTransform())
    {
        ClearMeshAndCollision();
        return false;
    }

    FWorldDensityChunkSnapshot Snapshot;
    if (!Excavation->BuildChunkSnapshot(ChunkCoordinate, Snapshot))
    {
        ClearMeshAndCollision();
        return false;
    }

    FWorldVoxelMeshData MeshData;
    FString MesherError;
    const FWorldMarchingTetrahedraMesher Mesher;
    if (!Mesher.BuildMesh(Snapshot, MeshData, MesherError)
        || MeshData.TriangleIndices.Num() / 3 > FMath::Clamp(MaxGeneratedTriangles, 1000, 1000000))
    {
        ClearMeshAndCollision();
        return false;
    }

    return PublishVoxelMeshData(MeshData, MesherError);
}

bool AExcavationTerrainChunkActor::PublishVoxelMeshData(
    const FWorldVoxelMeshData& MeshData,
    FString& OutError)
{
    OutError.Reset();
    UWorld* World = GetWorld();
    UExcavationVolumeSubsystem* Excavation =
        World ? World->GetSubsystem<UExcavationVolumeSubsystem>() : nullptr;
    if (!DynamicMeshComponent || !IsValid(Excavation) || !RefreshChunkWorldTransform())
    {
        OutError = TEXT("Voxel chunk cannot publish without its component, world, and density owner.");
        return false;
    }

    if (MeshData.ChunkCoordinate != ChunkCoordinate)
    {
        OutError = TEXT("Voxel mesh output belongs to a different chunk coordinate.");
        return false;
    }

    if (Excavation->GetDensityRevision() != MeshData.SourceRevision)
    {
        OutError = TEXT("Discarded stale voxel mesh; the authoritative density revision changed.");
        if (IsValid(RegisteredSubsystem))
        {
            RegisteredSubsystem->QueueChunkRebuild(ChunkCoordinate);
        }
        return false;
    }

    if (!MeshData.IsTopologyValid()
        || MeshData.TriangleIndices.Num() / 3 > FMath::Clamp(MaxGeneratedTriangles, 1000, 1000000))
    {
        OutError = TEXT("CPU voxel mesh output failed topology or triangle-budget validation.");
        return false;
    }

    if (IsValid(RegisteredSubsystem) && RegisteredSubsystem->IsRuntimeReady())
    {
        DynamicMeshComponent->bUseAsyncCooking =
            RegisteredSubsystem->GetRuntimeSettings().bAllowAsyncCollision;
    }

    TMap<int32, int32> MaterialSlotByBaseId;
    MaterialSlotByBaseId.Add(0, 0);
    DynamicMeshComponent->SetMaterial(
        0, SurfaceMaterial ? SurfaceMaterial.Get() : UMaterial::GetDefaultMaterial(MD_Surface));

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

    UE::Geometry::FDynamicMesh3 CandidateMesh;
    FString ConversionError;
    if (!ConvertMeshDataToDynamicMesh(MeshData, MaterialSlotByBaseId, CandidateMesh, ConversionError))
    {
        OutError = ConversionError.IsEmpty()
            ? TEXT("Unable to convert CPU voxel mesh to GeometryCore dynamic mesh.")
            : ConversionError;
        return false;
    }

    // All work before this point is a local candidate. Publish atomically once
    // structure, attributes, bounds and density revision have been checked.
    DynamicMeshComponent->SetMesh(MoveTemp(CandidateMesh));
    DynamicMeshComponent->ValidateMaterialSlots(true, false);
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
    return true;
}
