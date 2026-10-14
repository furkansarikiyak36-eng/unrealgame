#pragma once
#include "CoreMinimal.h"
#include "Core/StructuralGraph.h"
#include "StickConnectionGraph.generated.h"
UCLASS(ClassGroup=(Stick), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStickConnectionGraph : public UStructuralGraph
{
    GENERATED_BODY()
public:
    UStickConnectionGraph();
    UFUNCTION(BlueprintCallable) bool ConnectWoven(AActor* Other, FName MySocket, FName TheirSocket);
    UFUNCTION(BlueprintCallable) bool ConnectBound(AActor* Other, FName MySocket, FName TheirSocket, FName RopeType);
    UFUNCTION(BlueprintCallable) bool ConnectLashed(AActor* Other, FName MySocket, FName TheirSocket);
};
