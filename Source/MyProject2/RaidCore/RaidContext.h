#pragma once
#include "CoreMinimal.h"
#include "RaidTypes.h"
#include "RaidContext.generated.h"
USTRUCT(BlueprintType) struct FRaidContext {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadWrite) FName EventID;
 UPROPERTY(BlueprintReadWrite) ERaidEventType EventType=ERaidEventType::None;
 UPROPERTY(BlueprintReadWrite) ERaidLifecycleState Lifecycle=ERaidLifecycleState::Idle;
 UPROPERTY(BlueprintReadWrite) TArray<FPlayerStateSnapshot> Players;
 UPROPERTY(BlueprintReadWrite) FWorldSnapshot World;
 UPROPERTY(BlueprintReadWrite) float CapturedTime=0.f;
 UPROPERTY(BlueprintReadWrite) float EventStartTime=0.f;
 UPROPERTY(BlueprintReadWrite) int32 DifficultyStage=0;
 UPROPERTY(BlueprintReadWrite) int32 DifficultyScore=0;
 UPROPERTY(BlueprintReadWrite) bool bScoreFrozen=false;
 UPROPERTY(BlueprintReadWrite) bool bPlanFrozen=false;
 UPROPERTY(BlueprintReadWrite) bool bEventActive=false;
 void Reset(){ EventID=NAME_None; EventType=ERaidEventType::None; Lifecycle=ERaidLifecycleState::Idle; Players.Empty(); World=FWorldSnapshot(); CapturedTime=EventStartTime=0.f; DifficultyStage=DifficultyScore=0; bScoreFrozen=bPlanFrozen=bEventActive=false; }
};
