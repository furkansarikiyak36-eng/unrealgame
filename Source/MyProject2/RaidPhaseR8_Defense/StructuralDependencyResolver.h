#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructuralDependencyResolver.generated.h"
UCLASS() class MYPROJECT2_API UStructuralDependencyResolver : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) TArray<AActor*> GetDependentBlocks(AActor* Block) const; UFUNCTION(BlueprintCallable) int32 PropagateCollapse(AActor* Block); };
