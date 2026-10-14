// MaterialDebugHUD.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MaterialDebugHUD.generated.h"

UCLASS()
class MYPROJECT2_API AMaterialDebugHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void SetDebugTarget(AActor* Target);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebug = true;

protected:
    UPROPERTY() TWeakObjectPtr<AActor> DebugTarget;
};
