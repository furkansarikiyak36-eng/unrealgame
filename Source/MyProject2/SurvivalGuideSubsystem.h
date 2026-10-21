// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SurvivalGuideSubsystem.generated.h"

/** One line of the survival demo list. */
struct FSurvivalGoalLine
{
	FString Text;
	FString Hint;
	bool bDone = false;
};

/**
 * The survival demo: a checklist through every pillar of the game (fire, cold, food, farming, animals, clothes, ropes,
 * bridges, notes, defences, the first raid night), ticked off as the player does each thing. It runs whenever the world
 * scenario is laid out (normal play, not the mechanics test). Nothing is forced; it only shows the way.
 */
UCLASS()
class MYPROJECT2_API USurvivalGuideSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static USurvivalGuideSubsystem* Get(const UWorld* World);

	bool IsActive() const;
	void GetLines(TArray<FSurvivalGoalLine>& OutLines, int32& OutCurrent) const;
	int32 GetDoneCount() const;
	static int32 GetGoalCount();

	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsTickable() const override { return IsActive(); }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USurvivalGuideSubsystem, STATGROUP_Tickables); }
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void Poll();

	TArray<bool> Done;
	float PollTimer = 0.f;
	bool bRodeRope = false;
};
