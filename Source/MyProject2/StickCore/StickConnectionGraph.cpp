#include "StickConnectionGraph.h"
UStickConnectionGraph::UStickConnectionGraph() { PrimaryComponentTick.bCanEverTick = false; }
bool UStickConnectionGraph::ConnectWoven(AActor* Other, FName MySocket, FName TheirSocket) { return ConnectTo(Other, MySocket, TheirSocket, EConnectionType::Woven, true); }
bool UStickConnectionGraph::ConnectBound(AActor* Other, FName MySocket, FName TheirSocket, FName RopeType) { return ConnectTo(Other, MySocket, TheirSocket, EConnectionType::Bound, true); }
bool UStickConnectionGraph::ConnectLashed(AActor* Other, FName MySocket, FName TheirSocket) { return ConnectTo(Other, MySocket, TheirSocket, EConnectionType::Lashed, true); }
