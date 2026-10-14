#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaidTypes.h"
#include "TargetSourceResolver.generated.h"
UCLASS() class MYPROJECT2_API UTargetSourceResolver : public UObject { GENERATED_BODY() public: AActor* ResolveTargetPriority(const TArray<FTargetCandidate>& Candidates,AActor* ForcedTarget,AActor* CurrentTarget) const; };
