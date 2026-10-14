#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "NavGraphBuilder.generated.h"
UCLASS() class MYPROJECT2_API UNavGraph : public UObject { GENERATED_BODY() public: UPROPERTY(BlueprintReadWrite) TArray<FNavNode> Nodes; UPROPERTY(BlueprintReadWrite) float CellSize=200.f; int32 FindNearestNode(const FVector& Position) const; void AddNode(const FVector& Position); void ConnectNodes(int32 A,int32 B); void Clear(); };
UCLASS() class MYPROJECT2_API UNavGraphBuilder : public UObject { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadWrite) float BuildRadius=5000.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float CellSize=200.f; void BuildFromWorld(UWorld* World,const FVector& Center,UNavGraph* OutGraph); };
