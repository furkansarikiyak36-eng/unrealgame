#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LogTypes.h"
#include "LogConnectionGraph.generated.h"
class ALogActor;

UCLASS(ClassGroup=(Log), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API ULogConnectionGraph : public UActorComponent
{
    GENERATED_BODY()
public:
    ULogConnectionGraph();
    UFUNCTION(BlueprintCallable) bool ConnectTo(ALogActor* Other, FName MySocket, FName TheirSocket, bool bLoadBearing = true);
    UFUNCTION(BlueprintCallable) bool DisconnectFrom(ALogActor* Other);
    UFUNCTION(BlueprintPure) TArray<ALogActor*> GetConnectedLogs() const;
    UFUNCTION(BlueprintPure) bool HasPathToGround() const;
    UFUNCTION(BlueprintCallable) TArray<ALogActor*> SimulateWithout(ALogActor* Removed) const;
    UFUNCTION(BlueprintCallable) void RebuildGraph();
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) TArray<FLogConnection> Connections;
protected:
    ALogActor* GetOwnerLog() const;
};
