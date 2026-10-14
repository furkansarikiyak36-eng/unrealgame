#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RaidTypes.h"
#include "RaidStageTableAsset.generated.h"
USTRUCT(BlueprintType) struct FRaidStageConfig { GENERATED_BODY() UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 StageIndex=0; UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Threshold=0; UPROPERTY(EditAnywhere,BlueprintReadOnly) FDifficultyBudget Budget; UPROPERTY(EditAnywhere,BlueprintReadOnly) FName PlanID; };
UCLASS(BlueprintType) class MYPROJECT2_API URaidStageTableAsset : public UPrimaryDataAsset { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadOnly) FName TableName; UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FRaidStageConfig> Stages; UPROPERTY(EditAnywhere,BlueprintReadOnly) float LevelMultiplier=1.f; UPROPERTY(EditAnywhere,BlueprintReadOnly) float SurvivalTimeMultiplier=.1f; UPROPERTY(EditAnywhere,BlueprintReadOnly) float DeathPenaltyFactor=.5f; UPROPERTY(EditAnywhere,BlueprintReadOnly) float PartyWeightFirst=1.f; UPROPERTY(EditAnywhere,BlueprintReadOnly) float PartyWeightDecay=.8f; UFUNCTION(BlueprintCallable) bool ValidateTable(FString& OutError) const; const FRaidStageConfig* FindStageConfig(int32 StageIndex) const; };
