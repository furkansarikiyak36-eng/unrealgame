// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ForestGameMode.generated.h"

/**
 * Sets up the forest prototype, converts world trees/rocks to harvestables,
 * and spawns the comprehensive Log, Stone & Plaster engineering test demo showcase.
 */
UCLASS()
class MYPROJECT2_API AForestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AForestGameMode();

	virtual void StartPlay() override;

	/** Sets up the comprehensive Log, Stone & Plaster engineering test demo */
	UFUNCTION(BlueprintCallable, Category = "Forest|Demo")
	void SetupDemoShowcase();

private:
	void ConvertHarvestables();

	/** Boulders scaled above this stay as scenery (cliffs). */
	UPROPERTY(EditAnywhere, Category = "Forest")
	float MaxRockScale = 2.f;

	/** Spawns the interactive engineering test demo area in front of the player on start */
	UPROPERTY(EditAnywhere, Category = "Forest|Demo")
	bool bSpawnDemoShowcase = true;
};
