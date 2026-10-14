#include "StoneConnectionGraph.h"

UStoneConnectionGraph::UStoneConnectionGraph() { PrimaryComponentTick.bCanEverTick = false; }

bool UStoneConnectionGraph::ConnectStacked(AActor* Below, FName MySocket, FName TheirSocket)
{ return ConnectTo(Below, MySocket, TheirSocket, EConnectionType::Stacked, true); }

bool UStoneConnectionGraph::ConnectAdjacent(AActor* Side, FName MySocket, FName TheirSocket)
{ return ConnectTo(Side, MySocket, TheirSocket, EConnectionType::ButtJoint, true); }

bool UStoneConnectionGraph::ConnectArchVoussoir(AActor* Voussoir, FName MySocket, FName TheirSocket)
{ return ConnectTo(Voussoir, MySocket, TheirSocket, EConnectionType::ArchVoussoir, true); }

bool UStoneConnectionGraph::ConnectKeystone(AActor* Key, FName MySocket, FName TheirSocket)
{ return ConnectTo(Key, MySocket, TheirSocket, EConnectionType::ArchVoussoir, true); }
