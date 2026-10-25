#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WattleFenceComponent.generated.h"

class AStickActor;
class UStickDefinition;
class UStaticMeshComponent;

UCLASS(ClassGroup = (Defense), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UWattleFenceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWattleFenceComponent();

	UFUNCTION(BlueprintCallable, Category = "Defense")
	bool BuildFence(const TArray<FVector>& PostPositions, int32 WeaveRows);

	/** Adds a clay infill visual/collision panel to one installed weave section. */
	UFUNCTION(BlueprintCallable, Category = "Defense")
	bool AddClayFill(AStickActor* FenceSection);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense", meta = (ClampMin = "30.0"))
	float PostHeight = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense", meta = (ClampMin = "10.0"))
	float MinimumPostSpacing = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	TSubclassOf<AStickActor> StickClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	TObjectPtr<UStickDefinition> FenceStickDefinition = nullptr;

private:
	void RollbackPieces(const TArray<AStickActor*>& Pieces);

	UPROPERTY(VisibleInstanceOnly, Category = "Defense")
	TArray<TObjectPtr<AStickActor>> BuiltPieces;

	UPROPERTY(VisibleInstanceOnly, Category = "Defense")
	TMap<TWeakObjectPtr<AStickActor>, TObjectPtr<UStaticMeshComponent>> ClayFillBySection;
};
