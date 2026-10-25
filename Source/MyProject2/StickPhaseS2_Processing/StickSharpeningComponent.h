#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickSharpeningComponent.generated.h"

class AStickActor;

UENUM(BlueprintType)
enum class ESharpeningMode : uint8
{
	SingleEnd,
	DoubleEnd
};

UCLASS(ClassGroup = (Stick), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UStickSharpeningComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStickSharpeningComponent();

	/** Records one axe hit. Returns true when the hit was accepted; the shape changes on the final hit. */
	UFUNCTION(BlueprintCallable, Category = "Stick|Processing")
	bool SharpenStick(AStickActor* Stick, ESharpeningMode Mode = ESharpeningMode::SingleEnd);

	UFUNCTION(BlueprintPure, Category = "Stick|Processing")
	int32 GetRegisteredHits(const AStickActor* Stick) const;

	UFUNCTION(BlueprintPure, Category = "Stick|Processing")
	float GetSharpenProgress(const AStickActor* Stick, ESharpeningMode Mode = ESharpeningMode::SingleEnd) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stick|Processing", meta = (ClampMin = "1", ClampMax = "20"))
	int32 HitsRequired = 3;

private:
	struct FProgress
	{
		ESharpeningMode Mode = ESharpeningMode::SingleEnd;
		int32 Hits = 0;
	};
	TMap<TWeakObjectPtr<AStickActor>, FProgress> HitProgress;
};
