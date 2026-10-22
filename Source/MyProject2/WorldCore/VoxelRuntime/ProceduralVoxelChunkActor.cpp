#include "ProceduralVoxelChunkActor.h"

#include "ProceduralMeshComponent.h"

AProceduralVoxelChunkActor::AProceduralVoxelChunkActor()
{
    PrimaryActorTick.bCanEverTick = false;
    VoxelMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("VoxelMesh"));
    SetRootComponent(VoxelMesh);
    VoxelMesh->bUseAsyncCooking = false;
    VoxelMesh->SetCollisionProfileName(TEXT("BlockAll"));
    VoxelMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    VoxelMesh->SetCollisionObjectType(ECC_WorldStatic);
    VoxelMesh->SetCanEverAffectNavigation(true);
}

void AProceduralVoxelChunkActor::ApplySurface(const TArray<FVector>& Vertices,
    const TArray<int32>& Triangles, const TArray<FVector>& Normals, bool bCreateCollision)
{
    if (!VoxelMesh)
    {
        return;
    }

    VoxelMesh->ClearAllMeshSections();
    if (Vertices.IsEmpty() || Triangles.IsEmpty())
    {
        VoxelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return;
    }

    TArray<FVector2D> UV0;
    UV0.Reserve(Vertices.Num());
    for (const FVector& Vertex : Vertices)
    {
        UV0.Add(FVector2D(Vertex.X / 100.0f, Vertex.Y / 100.0f));
    }

    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    VoxelMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, Colors, Tangents,
        bCreateCollision, false);
    VoxelMesh->SetCollisionEnabled(bCreateCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    VoxelMesh->SetCollisionObjectType(ECC_WorldStatic);
    VoxelMesh->SetCanEverAffectNavigation(true);
}
