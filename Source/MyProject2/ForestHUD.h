// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ForestHUD.generated.h"

/** Lightweight canvas HUD: crosshair, target health, inventory, controls and messages. */
UCLASS()
class MYPROJECT2_API AForestHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawPanel(float X, float Y, float Width, float Height);
};
