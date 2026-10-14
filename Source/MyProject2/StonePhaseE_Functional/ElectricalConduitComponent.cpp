#include "ElectricalConduitComponent.h"
UElectricalConduitComponent::UElectricalConduitComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UElectricalConduitComponent::ConnectCable(AActor* From, AActor* To)
{
    if (!From || !To || From == To || MaxCableLength <= 0.0f) return false;
    if (FVector::Dist(From->GetActorLocation(), To->GetActorLocation()) > MaxCableLength) return false;
    if (!CableNodes.Contains(From)) CableNodes.Add(From);
    if (!CableNodes.Contains(To)) CableNodes.Add(To);
    return true;
}
bool UElectricalConduitComponent::DisconnectCable(AActor* Node)
{
    if (!Node) return false;
    return CableNodes.RemoveAll([Node](const TWeakObjectPtr<AActor>& N) { return N.Get() == Node; }) > 0;
}
bool UElectricalConduitComponent::AttachLight(AActor* Node)
{
    if (!Node) return false;
    if (!CableNodes.Contains(Node)) CableNodes.Add(Node);
    return true;
}
