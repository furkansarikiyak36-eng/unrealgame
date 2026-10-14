#include "Core/LogConnectionGraph.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/StructuralGraph.h"

ULogConnectionGraph::ULogConnectionGraph() { PrimaryComponentTick.bCanEverTick = false; }
ALogActor* ULogConnectionGraph::GetOwnerLog() const { return Cast<ALogActor>(GetOwner()); }

bool ULogConnectionGraph::ConnectTo(ALogActor* Other, FName MySocket, FName TheirSocket, bool bLoadBearing)
{
    ALogActor* OwnerLog = GetOwnerLog();
    if (!OwnerLog || !Other || Other == OwnerLog || MySocket.IsNone() || TheirSocket.IsNone()) return false;
    for (const FLogConnection& C : Connections) if (C.ConnectedLog.Get() == Other && C.MySocket == MySocket && C.TheirSocket == TheirSocket) return false;
    ULogConnectionGraph* OtherGraph = Other->FindComponentByClass<ULogConnectionGraph>();
    if (!OtherGraph) return false;
    FLogConnection A; A.ConnectedLog = Other; A.MySocket = MySocket; A.TheirSocket = TheirSocket; A.bLoadBearing = bLoadBearing;
    FLogConnection B; B.ConnectedLog = OwnerLog; B.MySocket = TheirSocket; B.TheirSocket = MySocket; B.bLoadBearing = bLoadBearing;
    Connections.Add(A); OtherGraph->Connections.Add(B);
    if (OwnerLog->StructuralGraph)
        OwnerLog->StructuralGraph->ConnectTo(Other, MySocket, TheirSocket, EConnectionType::ButtJoint, bLoadBearing);
    return true;
}

bool ULogConnectionGraph::DisconnectFrom(ALogActor* Other)
{
    if (!Other) return false;
    const int32 Removed = Connections.RemoveAll([Other](const FLogConnection& C) { return C.ConnectedLog.Get() == Other; });
    if (ULogConnectionGraph* G = Other->FindComponentByClass<ULogConnectionGraph>()) G->Connections.RemoveAll([this](const FLogConnection& C) { return C.ConnectedLog.Get() == GetOwner(); });
    if (ALogActor* OwnerLog = GetOwnerLog())
        if (OwnerLog->StructuralGraph) OwnerLog->StructuralGraph->DisconnectFrom(Other);
    return Removed > 0;
}

TArray<ALogActor*> ULogConnectionGraph::GetConnectedLogs() const
{
    TArray<ALogActor*> Out; for (const FLogConnection& C : Connections) if (ALogActor* L = Cast<ALogActor>(C.ConnectedLog.Get())) Out.AddUnique(L); return Out;
}

bool ULogConnectionGraph::HasPathToGround() const
{
    TSet<ALogActor*> Visited; TArray<ALogActor*> Stack; if (ALogActor* OwnerLog = GetOwnerLog()) Stack.Add(OwnerLog);
    while (Stack.Num()) { ALogActor* Current = Stack.Pop(EAllowShrinking::No); if (!Current || Visited.Contains(Current)) continue; Visited.Add(Current); if (Current->IsLogGrounded()) return true;
        if (ULogConnectionGraph* G = Current->FindComponentByClass<ULogConnectionGraph>()) for (const FLogConnection& C : G->Connections) if (C.bLoadBearing && C.ConnectionStrength > 0.01f) if (ALogActor* Next = Cast<ALogActor>(C.ConnectedLog.Get())) Stack.Add(Next);
    } return false;
}

TArray<ALogActor*> ULogConnectionGraph::SimulateWithout(ALogActor* Removed) const
{
    TArray<ALogActor*> Orphans; if (!Removed) return Orphans;
    TArray<ALogActor*> Seeds = Removed->FindComponentByClass<ULogConnectionGraph>() ? Removed->FindComponentByClass<ULogConnectionGraph>()->GetConnectedLogs() : TArray<ALogActor*>();
    TSet<ALogActor*> GlobalVisited;
    for (ALogActor* Seed : Seeds)
    {
        if (!Seed || Seed == Removed || GlobalVisited.Contains(Seed)) continue;
        TArray<ALogActor*> Component; TArray<ALogActor*> Stack{Seed}; bool bHasGround = false;
        while (Stack.Num())
        {
            ALogActor* Current = Stack.Pop(EAllowShrinking::No);
            if (!Current || Current == Removed || GlobalVisited.Contains(Current)) continue;
            GlobalVisited.Add(Current); Component.Add(Current); bHasGround |= Current->IsLogGrounded();
            if (ULogConnectionGraph* G = Current->FindComponentByClass<ULogConnectionGraph>())
                for (const FLogConnection& C : G->Connections) if (C.bLoadBearing && C.ConnectionStrength > 0.01f)
                    if (ALogActor* Next = Cast<ALogActor>(C.ConnectedLog.Get())) if (Next != Removed) Stack.Add(Next);
        }
        if (!bHasGround) Orphans.Append(Component);
    }
    return Orphans;
}

void ULogConnectionGraph::RebuildGraph() { Connections.RemoveAll([](const FLogConnection& C) { return !C.ConnectedLog.IsValid() || C.ConnectionStrength <= 0.0f; }); }
