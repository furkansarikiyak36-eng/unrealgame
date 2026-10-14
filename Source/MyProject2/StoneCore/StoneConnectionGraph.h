#pragma once
#include "CoreMinimal.h"
#include "Core/StructuralGraph.h"
#include "StoneConnectionGraph.generated.h"

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneConnectionGraph : public UStructuralGraph
{
    GENERATED_BODY()
public:
    UStoneConnectionGraph();
    UFUNCTION(BlueprintCallable) bool ConnectStacked(AActor* Below, FName MySocket, FName TheirSocket);
    UFUNCTION(BlueprintCallable) bool ConnectAdjacent(AActor* Side, FName MySocket, FName TheirSocket);
    UFUNCTION(BlueprintCallable) bool ConnectArchVoussoir(AActor* Voussoir, FName MySocket, FName TheirSocket);
    UFUNCTION(BlueprintCallable) bool ConnectKeystone(AActor* Key, FName MySocket, FName TheirSocket);
};
