#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructureDamageResolver.generated.h"
UCLASS() class MYPROJECT2_API UStructureDamageResolver : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintPure) bool IsAttackValid(AActor* Attacker,AActor* Target,float Range) const; UFUNCTION(BlueprintPure) float ComputeDamage(float RawDamage,float Resistance,float Multiplier) const; UFUNCTION(BlueprintCallable) bool ApplyDamageToBlock(AActor* Block,float Damage); UFUNCTION(BlueprintCallable) void UpdateWorldAfterBlockDestroyed(UWorld* World,AActor* Block); };
