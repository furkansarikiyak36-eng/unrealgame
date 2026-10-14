#pragma once
#include "Core/StructuralMaterialTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MaterialCore/MaterialTypes.h"
#include "StructuralGraph.generated.h"

UCLASS(ClassGroup=(Structural), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStructuralGraph : public UActorComponent
{
    GENERATED_BODY()
public:
    UStructuralGraph();

    UFUNCTION(BlueprintCallable)
    bool ConnectTo(AActor* Other, FName MySocket, FName TheirSocket,
                   EConnectionType Type = EConnectionType::ButtJoint,
                   bool bLoadBearing = true);
    UFUNCTION(BlueprintCallable) bool DisconnectFrom(AActor* Other);
    UFUNCTION(BlueprintCallable) bool MortarConnection(AActor* Other, float StrengthMultiplier = 1.5f);
    UFUNCTION(BlueprintPure) TArray<AActor*> GetConnectedNodes() const;
    UFUNCTION(BlueprintPure) bool HasPathToGround() const;
    UFUNCTION(BlueprintCallable) TArray<AActor*> SimulateWithout(AActor* Removed) const;
    UFUNCTION(BlueprintCallable) float ComputeTotalLoad() const;
    UFUNCTION(BlueprintCallable) void RebuildGraph();

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) TArray<FStructuralConnection> Connections;
};
