#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RaidTypes.h"
#include "HordePlanAsset.generated.h"
UCLASS(BlueprintType) class MYPROJECT2_API UHordePlanAsset : public UPrimaryDataAsset { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadOnly) FName PlanID; UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 StageIndex=0; UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FWavePlan> Waves; UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bUseStationKeeping=false; UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 StationKeepingMaxAlive=3; UFUNCTION(BlueprintCallable) float ComputeTotalDuration() const; UFUNCTION(BlueprintCallable) bool ValidatePlan(FString& OutError) const; };
UCLASS(BlueprintType) class MYPROJECT2_API UEnemyGroupLibrary : public UPrimaryDataAsset { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FEnemyGroupDefinition> Groups; const FEnemyGroupDefinition* FindGroup(FName GroupID) const; };
UCLASS(BlueprintType) class MYPROJECT2_API UEnemyLibrary : public UPrimaryDataAsset { GENERATED_BODY() public: UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FEnemyDefinition> Enemies; const FEnemyDefinition* FindEnemy(FName EnemyID) const; };
