// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LogWallBlueprint.generated.h"

class UInstancedStaticMeshComponent;
class ULineBatchComponent;
class UMaterialInstanceDynamic;

/**
 * A planned log wall, Sons of the Forest style: a see-through plan of stacked logs
 * that the player fills one log at a time, bottom row first. Walls started from
 * another wall's end are shifted half a row so corners interlock like a log cabin.
 */
UCLASS()
class MYPROJECT2_API ALogWallBlueprint : public AActor
{
	GENERATED_BODY()

public:
	ALogWallBlueprint();

	static constexpr int32 Rows = 4;

	/**
	 * Lays logs from Start towards End. With bFitExactly the logs are stretched slightly
	 * so the wall ends exactly at End (used when closing onto another wall).
	 * OutEnd receives where the wall really ends.
	 */
	static bool ComputeSlots(const FVector& Start, const FVector& End, float Phase, bool bFitExactly, TArray<FTransform>& OutSlots, FVector& OutEnd);

	void Init(const TArray<FTransform>& InSlots, const FVector& InStart, const FVector& InEnd, float InPhase);

	/** Puts the next log into the plan. Returns false when the wall is already complete. */
	bool AddLog();

	/** Applies chinking/daubing between the logs in this wall */
	UFUNCTION(BlueprintCallable, Category = "Log Wall")
	void ApplyChinking();

	/** Triggers Chaos physics collapse of all placed logs in this wall */
	UFUNCTION(BlueprintCallable, Category = "Log Wall|Physics")
	void CollapseWall(const FVector& ImpactDirection = FVector::ZeroVector);

	bool IsComplete() const { return Filled >= Slots.Num(); }
	int32 GetFilled() const { return Filled; }
	int32 GetTotal() const { return Slots.Num(); }
	const FVector& GetStart() const { return Start; }
	const FVector& GetEnd() const { return End; }
	float GetPhase() const { return Phase; }
	bool IsChinked() const { return bIsChinked; }
	const UInstancedStaticMeshComponent* GetGhosts() const { return Ghosts; }

private:
	void RefreshGhosts();
	void UpdateVisualColor();

	UPROPERTY(EditAnywhere, Category = "Engineering")
	bool bIsChinked = false;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	/** Remaining planned logs: hidden in game, only there so the player can aim at the plan. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> Ghosts;

	/** The remaining planned logs drawn as lines; the next log to add is drawn brighter. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULineBatchComponent> GhostLines;

	/** Logs already built, solid. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> Logs;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GhostMaterial;

	TArray<FTransform> Slots;
	int32 Filled = 0;
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	float Phase = 0.f;
};
