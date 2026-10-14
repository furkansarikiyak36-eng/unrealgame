#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RaidContext.h"
#include "RaidConditionGate.generated.h"
UCLASS(ClassGroup=(Raid),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API URaidConditionGate : public UActorComponent { GENERATED_BODY() public: URaidConditionGate(); UFUNCTION(BlueprintCallable) bool EvaluateGate(const FRaidContext& Context) const; UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bEnabled=true; UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bRequireNight=false; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MinPlayers=1; UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bRequireBase=true; };
