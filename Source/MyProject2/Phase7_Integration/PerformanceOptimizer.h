#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PerformanceOptimizer.generated.h"
UCLASS(Config=Game) class MYPROJECT2_API UPerformanceOptimizer:public UWorldSubsystem
{GENERATED_BODY() public: virtual void Initialize(FSubsystemCollectionBase& Collection)override; virtual void Deinitialize()override; UFUNCTION(BlueprintCallable) void OptimizeAll(); UPROPERTY(Config,EditAnywhere,Category="Performance") int32 MaxActivePhysicsLogs=50; };
