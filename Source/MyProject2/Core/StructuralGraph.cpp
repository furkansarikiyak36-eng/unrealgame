#include "StructuralGraph.h"
#include "StructuralNode.h"
#include "GameFramework/Actor.h"

UStructuralGraph::UStructuralGraph()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UStructuralGraph::ConnectTo(AActor* Other, FName MySocket, FName TheirSocket,
    EConnectionType Type, bool bLoadBearing)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Other || Other == Owner) return false;
    for (const FStructuralConnection& C : Connections)
        if (C.ConnectedNode.Get() == Other && C.MySocket == MySocket && C.TheirSocket == TheirSocket) return false;

    UStructuralGraph* OtherGraph = Other->FindComponentByClass<UStructuralGraph>();
    if (!OtherGraph) return false;
    if (!Owner->GetClass()->ImplementsInterface(UStructuralNode::StaticClass()) ||
        !Other->GetClass()->ImplementsInterface(UStructuralNode::StaticClass())) return false;

    const EMaterialType MyMat = IStructuralNode::Execute_GetMaterialType(Owner);
    const EMaterialType TheirMat = IStructuralNode::Execute_GetMaterialType(Other);
    const bool bCompatible = (MyMat == TheirMat) ||
        (MyMat == EMaterialType::Wood && TheirMat == EMaterialType::Stone) ||
        (MyMat == EMaterialType::Stone && TheirMat == EMaterialType::Wood) ||
        MyMat == EMaterialType::Mortar || TheirMat == EMaterialType::Mortar;
    if (!bCompatible) return false;

    FStructuralConnection NewConn;
    NewConn.ConnectedNode = Other;
    NewConn.MyMaterial = MyMat;
    NewConn.TheirMaterial = TheirMat;
    NewConn.ConnectionType = Type;
    NewConn.MySocket = MySocket;
    NewConn.TheirSocket = TheirSocket;
    NewConn.ConnectionStrength = 1.0f;
    NewConn.bLoadBearing = bLoadBearing;
    NewConn.bMortared = Type == EConnectionType::Mortared;
    Connections.Add(NewConn);

    {
        FStructuralConnection Back;
        Back.ConnectedNode = Owner;
        Back.MyMaterial = TheirMat;
        Back.TheirMaterial = MyMat;
        Back.ConnectionType = Type;
        Back.MySocket = TheirSocket;
        Back.TheirSocket = MySocket;
        Back.ConnectionStrength = 1.0f;
        Back.bLoadBearing = bLoadBearing;
        Back.bMortared = NewConn.bMortared;
        OtherGraph->Connections.Add(Back);
    }
    return true;
}

bool UStructuralGraph::DisconnectFrom(AActor* Other)
{
    if (!Other) return false;
    const int32 Removed = Connections.RemoveAll([Other](const FStructuralConnection& C)
    { return C.ConnectedNode.Get() == Other; });
    if (UStructuralGraph* OtherGraph = Other->FindComponentByClass<UStructuralGraph>())
    {
        AActor* Owner = GetOwner();
        OtherGraph->Connections.RemoveAll([Owner](const FStructuralConnection& C)
        { return C.ConnectedNode.Get() == Owner; });
    }
    return Removed > 0;
}

bool UStructuralGraph::MortarConnection(AActor* Other, float StrengthMultiplier)
{
    if (!Other || StrengthMultiplier <= 0.0f) return false;
    bool bFound = false;
    for (FStructuralConnection& C : Connections)
    {
        if (C.ConnectedNode.Get() == Other)
        {
            C.bMortared = true;
            C.ConnectionType = EConnectionType::Mortared;
            C.ConnectionStrength *= StrengthMultiplier;
            bFound = true;
        }
    }
    if (!bFound) return false;
    if (UStructuralGraph* OtherGraph = Other->FindComponentByClass<UStructuralGraph>())
    {
        for (FStructuralConnection& C : OtherGraph->Connections)
        {
            if (C.ConnectedNode.Get() == GetOwner())
            {
                C.bMortared = true;
                C.ConnectionType = EConnectionType::Mortared;
                C.ConnectionStrength *= StrengthMultiplier;
            }
        }
    }
    return true;
}

TArray<AActor*> UStructuralGraph::GetConnectedNodes() const
{
    TArray<AActor*> Result;
    for (const FStructuralConnection& C : Connections)
        if (AActor* Node = C.ConnectedNode.Get()) Result.Add(Node);
    return Result;
}

bool UStructuralGraph::HasPathToGround() const
{
    TSet<AActor*> Visited;
    TArray<AActor*> Stack;
    if (GetOwner()) Stack.Add(GetOwner());
    while (Stack.Num() > 0)
    {
        AActor* Current = Stack.Pop();
        if (!Current || Visited.Contains(Current)) continue;
        Visited.Add(Current);
        if (Current->GetClass()->ImplementsInterface(UStructuralNode::StaticClass()) &&
            IStructuralNode::Execute_IsGrounded(Current)) return true;
        if (UStructuralGraph* G = Current->FindComponentByClass<UStructuralGraph>())
            for (const FStructuralConnection& C : G->Connections)
                if (C.bLoadBearing)
                    if (AActor* Next = C.ConnectedNode.Get()) Stack.Add(Next);
    }
    return false;
}

TArray<AActor*> UStructuralGraph::SimulateWithout(AActor* Removed) const
{
    TArray<AActor*> Orphans;
    if (!Removed) return Orphans;

    TSet<AActor*> Visited;
    TArray<AActor*> Seeds;
    if (UStructuralGraph* RemovedGraph = Removed->FindComponentByClass<UStructuralGraph>())
    {
        for (const FStructuralConnection& C : RemovedGraph->Connections)
            if (C.bLoadBearing && C.ConnectionStrength > 0.01f)
                if (AActor* Node = C.ConnectedNode.Get()) if (Node != Removed) Seeds.AddUnique(Node);
    }

    for (AActor* Seed : Seeds)
    {
        if (!Seed || Seed == Removed || Visited.Contains(Seed)) continue;
        TArray<AActor*> Component;
        TArray<AActor*> Stack;
        Stack.Add(Seed);
        bool bHasGround = false;

        while (Stack.Num() > 0)
        {
            AActor* Current = Stack.Pop(EAllowShrinking::No);
            if (!Current || Current == Removed || Visited.Contains(Current)) continue;
            Visited.Add(Current);
            Component.Add(Current);

            if (Current->GetClass()->ImplementsInterface(UStructuralNode::StaticClass()) &&
                IStructuralNode::Execute_IsGrounded(Current)) bHasGround = true;

            if (UStructuralGraph* G = Current->FindComponentByClass<UStructuralGraph>())
            {
                for (const FStructuralConnection& C : G->Connections)
                {
                    if (!C.bLoadBearing || C.ConnectionStrength <= 0.01f) continue;
                    AActor* Next = C.ConnectedNode.Get();
                    if (Next && Next != Removed && !Visited.Contains(Next)) Stack.Add(Next);
                }
            }
        }
        if (!bHasGround) Orphans.Append(Component);
    }
    return Orphans;
}

float UStructuralGraph::ComputeTotalLoad() const
{
    float Total = 0.0f;
    for (AActor* Node : GetConnectedNodes())
        if (Node && Node->GetClass()->ImplementsInterface(UStructuralNode::StaticClass()))
            Total += IStructuralNode::Execute_GetStructuralMass(Node);
    return Total;
}

void UStructuralGraph::RebuildGraph()
{
    Connections.RemoveAll([](const FStructuralConnection& C) { return !C.ConnectedNode.IsValid(); });
}
