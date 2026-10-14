#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RaidDebugHUD.generated.h"
UCLASS() class MYPROJECT2_API ARaidDebugHUD : public AHUD { GENERATED_BODY() public: virtual void DrawHUD() override; UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bShowDebug=true; UPROPERTY(EditAnywhere,BlueprintReadWrite) FString DebugText; };
