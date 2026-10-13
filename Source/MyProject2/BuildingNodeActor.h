// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingNodeActor.generated.h"

class UStructuralIntegritySubsystem;

/**
 * Base building node actor for all structural elements (Logs, Stone Masonry, Trusses, Bucks).
 * Integrates with UStructuralIntegritySubsystem to propagate gravity & lateral loads,
 * evaluate structural capacity, and trigger cascading physics collapses.
 */
UCLASS(Abstract)
class MYPROJECT2_API ABuildingNodeActor : public AActor
{
	GENERATED_BODY()

public:
	ABuildingNodeActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Deals structural damage. Returns remaining structural health. */
	virtual float ApplyDamage(float Amount, const FVector& Direction);

	/** Triggers dynamic Chaos Physics collapse of this building node */
	UFUNCTION(BlueprintCallable, Category = "Structural Integrity|Physics")
	virtual void TriggerChaosCollapse(const FVector& Impulse = FVector::ZeroVector);

	/** Adds a child node that rests on top of this node */
	void AddSupportingNode(ABuildingNodeActor* Child);

	/** Removes a child node */
	void RemoveSupportingNode(ABuildingNodeActor* Child);

	/** Sets parent nodes that physically support this node from below */
	void AddSupportedByNode(ABuildingNodeActor* Parent);

	// --- Structural Getters & Setters ---
	float GetSelfWeight() const { return SelfWeight; }
	float GetCurrentLoadKN() const { return CurrentLoadKN; }
	void SetCurrentLoadKN(float InLoad) { CurrentLoadKN = InLoad; }
	float GetCompressiveCapacityKN() const { return CompressiveCapacityKN; }
	bool IsGrounded() const { return bIsGrounded; }
	void SetGrounded(bool bGrounded) { bIsGrounded = bGrounded; }
	bool IsCollapsed() const { return bIsCollapsed; }

	const TArray<TWeakObjectPtr<ABuildingNodeActor>>& GetSupportingNodes() const { return SupportingNodes; }
	const TArray<TWeakObjectPtr<ABuildingNodeActor>>& GetSupportedByNodes() const { return SupportedByNodes; }

protected:
	/** Self weight in kilograms */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural Integrity")
	float SelfWeight;

	/** Maximum compressive vertical load this node can bear (kN) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural Integrity")
	float CompressiveCapacityKN;

	/** Current accumulated vertical load from nodes resting above (kN) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structural Integrity")
	float CurrentLoadKN;

	/** Structural health points before failure */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural Integrity")
	float StructuralHealth;

	/** True if directly anchored to ground/bedrock */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structural Integrity")
	bool bIsGrounded;

	/** True if actor has already suffered collapse */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structural Integrity")
	bool bIsCollapsed;

	/** Nodes resting on top of this node (dependent on this for support) */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ABuildingNodeActor>> SupportingNodes;

	/** Nodes directly beneath this node providing support */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ABuildingNodeActor>> SupportedByNodes;
};
