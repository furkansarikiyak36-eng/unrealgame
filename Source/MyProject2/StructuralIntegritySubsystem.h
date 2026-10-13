// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StructuralIntegritySubsystem.generated.h"

class ABuildingNodeActor;

/**
 * World subsystem managing structural integrity, load distribution,
 * and cascading collapse simulations for log, stone, and plaster constructions.
 */
UCLASS()
class MYPROJECT2_API UStructuralIntegritySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UStructuralIntegritySubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** Registers a building node with the structural graph */
	void RegisterNode(ABuildingNodeActor* Node);

	/** Unregisters a building node from the graph */
	void UnregisterNode(ABuildingNodeActor* Node);

	/** Recomputes support relationships and propagates vertical loads across the network */
	UFUNCTION(BlueprintCallable, Category = "Structural Integrity")
	void RecalculateLoadNetwork();

	/**
	 * Propagates catastrophic failure upwards from a failed node.
	 * Any nodes resting on top that have no remaining grounded support path will collapse.
	 */
	UFUNCTION(BlueprintCallable, Category = "Structural Integrity")
	void TriggerCascadeFailure(ABuildingNodeActor* OriginNode, const FVector& InitialImpulse = FVector::ZeroVector);

	/** Checks if a node has a valid structural path to the ground / bedrock */
	bool HasPathToGround(ABuildingNodeActor* Node, TSet<ABuildingNodeActor*>& Visited) const;

private:
	/** All active structural building nodes */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ABuildingNodeActor>> ActiveNodes;

	/** Time accumulator for periodic stability evaluation */
	float TimeSinceLastEvaluation;

	static constexpr float EvaluationInterval = 1.0f; // Check load distribution every 1s
};
