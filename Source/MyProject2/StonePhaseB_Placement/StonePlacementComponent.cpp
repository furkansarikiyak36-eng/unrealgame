#include "StonePlacementComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StonePhaseB_Placement/StoneCarryComponent.h"
#include "StoneCore/StoneDefinition.h"
#include "StoneCore/StoneConnectionGraph.h"
#include "Core/StructuralGraph.h"
#include "Core/StructuralNode.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Math/RotationMatrix.h"

namespace
{
    void CollectValidPlacementOptions(
        const AStoneActor* Stone,
        TArray<int32>& OutFaceIndices,
        TArray<int32>& OutSocketIndices)
    {
        OutFaceIndices.Reset();
        OutSocketIndices.Reset();

        const UStoneDefinition* Definition = Stone ? Stone->GetDefinition() : nullptr;
        if (!Definition)
        {
            return;
        }

        TSet<FName> OccupiedLocalSockets;
        TSet<FName> OccupiedLocalFaces;
        if (Stone->ConnectionGraph)
        {
            for (const FStructuralConnection& Connection : Stone->ConnectionGraph->Connections)
            {
                // Connections are stored on both endpoints; MySocket is the socket local to this actor.
                if (Connection.MySocket.IsNone())
                {
                    continue;
                }
                OccupiedLocalSockets.Add(Connection.MySocket);
                OccupiedLocalFaces.Add(Connection.MySocket);
            }
        }

        for (int32 Index = 0; Index < Definition->Faces.Num(); ++Index)
        {
            const FStoneFace& Face = Definition->Faces[Index];
            if (Face.FaceName.IsNone() || Face.LocalNormal.ContainsNaN() ||
                Face.LocalNormal.IsNearlyZero() || OccupiedLocalFaces.Contains(Face.FaceName))
            {
                continue;
            }
            // Per-actor occupancy comes from the canonical graph, never the shared data asset.
            OutFaceIndices.Add(Index);
        }

        for (int32 Index = 0; Index < Definition->Sockets.Num(); ++Index)
        {
            const FStoneSocket& Socket = Definition->Sockets[Index];
            if (Socket.SocketName.IsNone() || Socket.LocalPosition.ContainsNaN() ||
                Socket.LocalRotation.ContainsNaN() || !FMath::IsFinite(Socket.SocketRadius) ||
                Socket.SocketRadius <= 0.0f || OccupiedLocalSockets.Contains(Socket.SocketName))
            {
                continue;
            }
            OutSocketIndices.Add(Index);
        }
    }
}

UStonePlacementComponent::UStonePlacementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

bool UStonePlacementComponent::BeginPlacement(AStoneActor* Stone)
{
    if (PreviewStone || !IsValid(Stone) || !Stone->GetDefinition() || !GetOwner())
    {
        return false;
    }

    const EStructuralNodeState State = Stone->GetStructuralState_Implementation();
    if (State != EStructuralNodeState::Loose && State != EStructuralNodeState::Carried)
    {
        return false;
    }

    OriginalActorTransform = Stone->GetActorTransform();
    OriginalStructuralState = State;
    OriginalAttachComponent.Reset();
    OriginalAttachSocket = NAME_None;

    if (USceneComponent* Root = Stone->GetRootComponent())
    {
        OriginalAttachComponent = Root->GetAttachParent();
        OriginalAttachSocket = Root->GetAttachSocketName();
    }

    if (State == EStructuralNodeState::Loose &&
        !Stone->SetStructuralState_Implementation(EStructuralNodeState::Carried))
    {
        return false;
    }

    // If the stone was held, detach it from the hand/shoulder while preserving its state.
    Stone->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    PreviewStone = Stone;
    bAiming = true;
    bHasPlacementTarget = false;
    TargetSurfaceActor.Reset();
    PlacementOptionIndex = INDEX_NONE;
    ManualYawDegrees = 0.0f;
    TargetTransform = OriginalActorTransform;
    TargetSurfacePoint = OriginalActorTransform.GetLocation();
    TargetSurfaceNormal = FVector::UpVector;
    return true;
}

bool UStonePlacementComponent::CancelPlacement()
{
    AStoneActor* Stone = PreviewStone;
    if (!IsValid(Stone))
    {
        PreviewStone = nullptr;
        bAiming = false;
        bHasPlacementTarget = false;
        TargetSurfaceActor.Reset();
        PlacementOptionIndex = INDEX_NONE;
        return false;
    }

    Stone->SetActorTransform(OriginalActorTransform, false, nullptr, ETeleportType::TeleportPhysics);

    if (USceneComponent* AttachParent = OriginalAttachComponent.Get())
    {
        if (Stone->GetStructuralState_Implementation() != OriginalStructuralState &&
            !Stone->SetStructuralState_Implementation(OriginalStructuralState))
        {
            return false;
        }
        const FAttachmentTransformRules RestoreRules =
            (!OriginalAttachSocket.IsNone() && AttachParent->DoesSocketExist(OriginalAttachSocket))
                ? FAttachmentTransformRules::SnapToTargetNotIncludingScale
                : FAttachmentTransformRules::KeepWorldTransform;
        Stone->AttachToComponent(AttachParent, RestoreRules, OriginalAttachSocket);
    }
    else if (Stone->GetStructuralState_Implementation() != OriginalStructuralState)
    {
        if (!Stone->SetStructuralState_Implementation(OriginalStructuralState))
        {
            return false;
        }
    }

    PreviewStone = nullptr;
    bAiming = false;
    bHasPlacementTarget = false;
    TargetSurfaceActor.Reset();
    PlacementOptionIndex = INDEX_NONE;
    ManualYawDegrees = 0.0f;
    OriginalAttachComponent.Reset();
    OriginalAttachSocket = NAME_None;
    return true;
}

void UStonePlacementComponent::EnterAimMode()
{
    bAiming = true;
}

void UStonePlacementComponent::ExitAimMode()
{
    if (PreviewStone)
    {
        CancelPlacement();
        return;
    }

    bAiming = false;
    bHasPlacementTarget = false;
    TargetSurfaceActor.Reset();
    PlacementOptionIndex = INDEX_NONE;
    ManualYawDegrees = 0.0f;
}

bool UStonePlacementComponent::UpdatePlacementTarget(
    FVector SurfacePoint, FVector SurfaceNormal, AActor* SurfaceActor)
{
    if (!IsValid(PreviewStone) || !bAiming || SurfacePoint.ContainsNaN() ||
        SurfaceNormal.ContainsNaN() || SurfaceNormal.IsNearlyZero())
    {
        bHasPlacementTarget = false;
        TargetSurfaceActor.Reset();
        return false;
    }

    TargetSurfacePoint = SurfacePoint;
    TargetSurfaceNormal = SurfaceNormal.GetSafeNormal();
    TargetSurfaceActor = IsValid(SurfaceActor) ? SurfaceActor : nullptr;
    TargetTransform.SetLocation(TargetSurfacePoint);
    TargetTransform.SetRotation(FRotationMatrix::MakeFromZ(TargetSurfaceNormal).ToQuat());
    bHasPlacementTarget = true;
    ApplySelectedPlacementOption();
    return true;
}

void UStonePlacementComponent::CycleFace()
{
    if (!PreviewStone)
    {
        return;
    }

    TArray<int32> FaceIndices;
    TArray<int32> SocketIndices;
    CollectValidPlacementOptions(PreviewStone, FaceIndices, SocketIndices);

    const int32 OptionCount = FaceIndices.Num() + SocketIndices.Num();
    if (OptionCount <= 0)
    {
        PlacementOptionIndex = INDEX_NONE;
        return;
    }

    PlacementOptionIndex = PlacementOptionIndex == INDEX_NONE
        ? 0
        : (PlacementOptionIndex + 1) % OptionCount;

    ApplySelectedPlacementOption();
}

void UStonePlacementComponent::RotateStone(float Delta)
{
    if (!FMath::IsFinite(Delta) || FMath::IsNearlyZero(Delta))
    {
        return;
    }

    ManualYawDegrees = FMath::Fmod(ManualYawDegrees + Delta * 15.0f, 360.0f);
    ApplySelectedPlacementOption();
}

void UStonePlacementComponent::ApplySelectedPlacementOption()
{
    if (!PreviewStone || !bHasPlacementTarget)
    {
        return;
    }

    UStoneDefinition* Definition = PreviewStone->GetDefinition();
    TArray<int32> FaceIndices;
    TArray<int32> SocketIndices;
    CollectValidPlacementOptions(PreviewStone, FaceIndices, SocketIndices);

    const int32 OptionCount = FaceIndices.Num() + SocketIndices.Num();
    if (OptionCount > 0 && PlacementOptionIndex != INDEX_NONE &&
        (PlacementOptionIndex < 0 || PlacementOptionIndex >= OptionCount))
    {
        PlacementOptionIndex = 0;
    }
    if (OptionCount <= 0)
    {
        PlacementOptionIndex = INDEX_NONE;
    }

    const FVector SurfaceNormal = TargetSurfaceNormal.GetSafeNormal();
    FQuat BaseRotation = FRotationMatrix::MakeFromZ(SurfaceNormal).ToQuat();
    FVector PlacementLocation = TargetSurfacePoint;
    bool bSelectedSocket = false;
    const FStoneSocket* SelectedSocket = nullptr;

    if (PlacementOptionIndex != INDEX_NONE && PlacementOptionIndex < FaceIndices.Num())
    {
        const FStoneFace& Face = Definition->Faces[FaceIndices[PlacementOptionIndex]];
        const FVector WorldFaceNormal = BaseRotation.RotateVector(Face.LocalNormal.GetSafeNormal());
        const FQuat AlignRotation = FQuat::FindBetweenNormals(
            WorldFaceNormal.GetSafeNormal(), -SurfaceNormal);
        BaseRotation = AlignRotation * BaseRotation;
    }
    else if (PlacementOptionIndex != INDEX_NONE && PlacementOptionIndex >= FaceIndices.Num())
    {
        const int32 SocketOptionIndex = PlacementOptionIndex - FaceIndices.Num();
        if (SocketOptionIndex >= 0 && SocketOptionIndex < SocketIndices.Num())
        {
            SelectedSocket = &Definition->Sockets[SocketIndices[SocketOptionIndex]];
            const FVector LocalSocketNormal = SelectedSocket->LocalRotation.RotateVector(FVector::UpVector);
            const FVector WorldSocketNormal = BaseRotation.RotateVector(LocalSocketNormal.GetSafeNormal());
            const FQuat AlignRotation = FQuat::FindBetweenNormals(
                WorldSocketNormal.GetSafeNormal(), -SurfaceNormal);
            BaseRotation = AlignRotation * BaseRotation;
            bSelectedSocket = true;
        }
    }

    const FQuat YawRotation(SurfaceNormal, FMath::DegreesToRadians(ManualYawDegrees));
    const FQuat FinalRotation = YawRotation * BaseRotation;
    if (bSelectedSocket && SelectedSocket)
    {
        const FVector ScaledSocketPosition = TargetTransform.GetScale3D() * SelectedSocket->LocalPosition;
        PlacementLocation = TargetSurfacePoint - FinalRotation.RotateVector(ScaledSocketPosition);
    }
    else
    {
        // Place the physical mesh on the plane instead of centring its pivot on the hit point.
        const UStaticMeshComponent* MeshComponent =
            Cast<UStaticMeshComponent>(PreviewStone->GetRootComponent());
        const UStaticMesh* StaticMesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
        if (StaticMesh)
        {
            const FBox LocalBounds = StaticMesh->GetBoundingBox();
            const FVector Scale = TargetTransform.GetScale3D();
            const FVector LocalCenter = LocalBounds.GetCenter() * Scale;
            const FVector LocalExtent = LocalBounds.GetExtent() * Scale.GetAbs();
            const FVector AxisX = FinalRotation.RotateVector(FVector::ForwardVector);
            const FVector AxisY = FinalRotation.RotateVector(FVector::RightVector);
            const FVector AxisZ = FinalRotation.RotateVector(FVector::UpVector);
            const float SupportExtent =
                FMath::Abs(FVector::DotProduct(SurfaceNormal, AxisX)) * LocalExtent.X +
                FMath::Abs(FVector::DotProduct(SurfaceNormal, AxisY)) * LocalExtent.Y +
                FMath::Abs(FVector::DotProduct(SurfaceNormal, AxisZ)) * LocalExtent.Z;
            const float CenterProjection = FVector::DotProduct(
                SurfaceNormal, FinalRotation.RotateVector(LocalCenter));
            PlacementLocation += SurfaceNormal * (SupportExtent - CenterProjection);
        }
    }

    TargetTransform.SetLocation(PlacementLocation);
    TargetTransform.SetRotation(FinalRotation);
    PreviewStone->SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

bool UStonePlacementComponent::IsPlacementClear(const FTransform& CandidateTransform) const
{
    if (!GetWorld() || !IsValid(PreviewStone))
    {
        return false;
    }

    const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(PreviewStone->GetRootComponent());
    const UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Primitive);
    if (!Primitive || !MeshComponent || !MeshComponent->GetStaticMesh() ||
        !Primitive->IsQueryCollisionEnabled())
    {
        return false;
    }

    FCollisionObjectQueryParams ObjectTypes;
    ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectTypes.AddObjectTypesToQuery(ECC_PhysicsBody);
    ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);

    FComponentQueryParams QueryParams(SCENE_QUERY_STAT(StonePlacementClearance), false, GetOwner());
    QueryParams.AddIgnoredActor(PreviewStone);
    if (AActor* SurfaceActor = TargetSurfaceActor.Get())
    {
        // The traced support actor is ignored so the intended face may touch it.
        // Actual socket compatibility / structural linking remains a separate graph gate.
        QueryParams.AddIgnoredActor(SurfaceActor);
    }

    TArray<FOverlapResult> Overlaps;
    const bool bHasOverlap = GetWorld()->ComponentOverlapMulti(
        Overlaps,
        Primitive,
        CandidateTransform.GetLocation(),
        CandidateTransform.GetRotation(),
        QueryParams,
        ObjectTypes);
    return !bHasOverlap;
}

bool UStonePlacementComponent::TryConnectStructuralTarget(
    AStoneActor* StoneToPlace, bool& bOutCreatedConnection)
{
    bOutCreatedConnection = false;
    if (!IsValid(StoneToPlace))
    {
        return false;
    }

    AActor* Target = TargetSurfaceActor.Get();
    if (!IsValid(Target) ||
        !Target->GetClass()->ImplementsInterface(UStructuralNode::StaticClass()))
    {
        // Terrain/static scenery can support a stone physically without becoming a structural node.
        return true;
    }

    UStructuralGraph* MyGraph = StoneToPlace->FindComponentByClass<UStructuralGraph>();
    UStructuralGraph* TargetGraph = Target->FindComponentByClass<UStructuralGraph>();
    if (!MyGraph || !TargetGraph)
    {
        // Fail closed: an actor that participates in structural simulation must expose a graph.
        return false;
    }

    FName MySocket = NAME_None;
    FName TheirSocket = NAME_None;
    EConnectionType ConnectionType = TargetSurfaceNormal.Z >= 0.7f
        ? EConnectionType::Stacked
        : EConnectionType::ButtJoint;

    // If the player explicitly selected a source socket, require a nearby authored socket on
    // the target stone. Do not mark a mismatched socket as a load-bearing connection.
    UStoneDefinition* MyDefinition = StoneToPlace->GetDefinition();
    if (!MyDefinition)
    {
        return false;
    }

    TArray<int32> FaceIndices;
    TArray<int32> SocketIndices;
    CollectValidPlacementOptions(StoneToPlace, FaceIndices, SocketIndices);
    const int32 SocketOffset = PlacementOptionIndex - FaceIndices.Num();
    const bool bSelectedSocket = PlacementOptionIndex != INDEX_NONE &&
        SocketOffset >= 0 && SocketOffset < SocketIndices.Num();

    if (bSelectedSocket)
    {
        AStoneActor* TargetStone = Cast<AStoneActor>(Target);
        UStoneDefinition* TheirDefinition = TargetStone ? TargetStone->GetDefinition() : nullptr;
        if (!TargetStone || !TheirDefinition)
        {
            return false;
        }

        const FStoneSocket& SourceSocket = MyDefinition->Sockets[SocketIndices[SocketOffset]];
        const FVector SourceSocketWorld = TargetTransform.TransformPosition(SourceSocket.LocalPosition);
        float BestDistanceSq = TNumericLimits<float>::Max();
        int32 BestTargetSocket = INDEX_NONE;
        for (int32 Index = 0; Index < TheirDefinition->Sockets.Num(); ++Index)
        {
            const FStoneSocket& Candidate = TheirDefinition->Sockets[Index];
            if (Candidate.SocketName.IsNone() || Candidate.LocalPosition.ContainsNaN() ||
                Candidate.LocalRotation.ContainsNaN() || !FMath::IsFinite(Candidate.SocketRadius) ||
                Candidate.SocketRadius <= 0.0f)
            {
                continue;
            }

            bool bSocketOccupied = false;
            for (const FStructuralConnection& Existing : TargetGraph->Connections)
            {
                if (Existing.MySocket == Candidate.SocketName)
                {
                    bSocketOccupied = true;
                    break;
                }
            }
            if (bSocketOccupied)
            {
                continue;
            }

            const FVector CandidateWorld = TargetStone->GetActorTransform().TransformPosition(Candidate.LocalPosition);
            const float DistanceSq = FVector::DistSquared(SourceSocketWorld, CandidateWorld);
            const float AllowedDistance = FMath::Max(2.0f,
                FMath::Max(0.0f, SourceSocket.SocketRadius) + FMath::Max(0.0f, Candidate.SocketRadius));
            if (DistanceSq <= FMath::Square(AllowedDistance) && DistanceSq < BestDistanceSq)
            {
                BestDistanceSq = DistanceSq;
                BestTargetSocket = Index;
            }
        }

        if (BestTargetSocket == INDEX_NONE)
        {
            return false;
        }

        MySocket = SourceSocket.SocketName;
        TheirSocket = TheirDefinition->Sockets[BestTargetSocket].SocketName;
        ConnectionType = EConnectionType::Stacked;
    }

    // If the same relationship already exists, retain it rather than asking ConnectTo to duplicate it.
    for (const FStructuralConnection& Existing : MyGraph->Connections)
    {
        if (Existing.ConnectedNode.Get() == Target &&
            Existing.MySocket == MySocket &&
            Existing.TheirSocket == TheirSocket &&
            Existing.bLoadBearing)
        {
            return true;
        }
    }

    bOutCreatedConnection = MyGraph->ConnectTo(
        Target, MySocket, TheirSocket, ConnectionType, true);
    return bOutCreatedConnection;
}

bool UStonePlacementComponent::ConfirmPlacement()
{
    if (!IsValid(PreviewStone) || !bAiming || !bHasPlacementTarget ||
        !IsPlacementClear(TargetTransform))
    {
        return false;
    }

    AStoneActor* StoneToPlace = PreviewStone;
    StoneToPlace->SetActorTransform(TargetTransform, false, nullptr, ETeleportType::TeleportPhysics);

    bool bCreatedStructuralConnection = false;
    if (!TryConnectStructuralTarget(StoneToPlace, bCreatedStructuralConnection))
    {
        // A structural actor is never treated as supported unless its graph accepted the link.
        return false;
    }

    if (!StoneToPlace->SetStructuralState_Implementation(EStructuralNodeState::Installed))
    {
        if (bCreatedStructuralConnection)
        {
            if (UStructuralGraph* Graph = StoneToPlace->FindComponentByClass<UStructuralGraph>())
            {
                if (AActor* Target = TargetSurfaceActor.Get())
                {
                    Graph->DisconnectFrom(Target);
                }
            }
        }
        CancelPlacement();
        return false;
    }

    if (GetOwner())
    {
        if (UStoneCarryComponent* Carry = GetOwner()->FindComponentByClass<UStoneCarryComponent>())
        {
            // A loose stone can be placed without the carry component owning it.
            // If it was the carried stone, clear that reference only after installation succeeded.
            Carry->ReleaseForPlacement(StoneToPlace);
        }
    }

    PlacementHistory.Add(StoneToPlace);
    PreviewStone = nullptr;
    bAiming = false;
    bHasPlacementTarget = false;
    TargetSurfaceActor.Reset();
    PlacementOptionIndex = INDEX_NONE;
    ManualYawDegrees = 0.0f;
    OriginalAttachComponent.Reset();
    OriginalAttachSocket = NAME_None;
    return true;
}

bool UStonePlacementComponent::UndoLastPlacement()
{
    if (PlacementHistory.Num() == 0)
    {
        return false;
    }

    AStoneActor* Last = PlacementHistory.Pop().Get();
    return Last && Last->SetStructuralState_Implementation(EStructuralNodeState::Loose);
}

void UStonePlacementComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bAiming || !PreviewStone || !GetOwner() || !GetWorld())
    {
        return;
    }

    FVector Start = FVector::ZeroVector;
    FRotator ViewRotation = FRotator::ZeroRotator;
    GetOwner()->GetActorEyesViewPoint(Start, ViewRotation);
    if (Start.ContainsNaN() || ViewRotation.ContainsNaN())
    {
        bHasPlacementTarget = false;
        return;
    }

    const FVector End = Start + ViewRotation.Vector() * ReachDistance;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StonePlacementTrace), false, GetOwner());
    Params.AddIgnoredActor(PreviewStone);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        UpdatePlacementTarget(Hit.ImpactPoint, Hit.ImpactNormal, Hit.GetActor());
    }
    else
    {
        bHasPlacementTarget = false;
        TargetSurfaceActor.Reset();
        TargetTransform = OriginalActorTransform;
        PreviewStone->SetActorTransform(OriginalActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
    }
}
