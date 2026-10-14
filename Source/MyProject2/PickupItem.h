// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestTypes.h"
#include "PickupItem.generated.h"

class UStaticMeshComponent;

/**
 * A physics item lying in the world. Pickable items go to the player's inventory;
 * cosmetic ones (bark, chips, pebbles) just fall and disappear.
 * Physics is switched off once the item comes to rest to keep the cost low.
 */
UCLASS()
class MYPROJECT2_API APickupItem : public AActor
{
	GENERATED_BODY()

public:
	APickupItem();

	static APickupItem* Spawn(UWorld* World, EForestItem Item, int32 Count, const FTransform& Transform, const FVector& Impulse);
	static APickupItem* SpawnCosmetic(UWorld* World, UStaticMesh* Mesh, const FTransform& Transform, const FVector& Impulse);

	virtual void Tick(float DeltaSeconds) override;

	bool IsPickable() const { return Item != EForestItem::None; }
	EForestItem GetItem() const { return Item; }

	/** Held by the player: no physics or collision while attached. */
	void BeginCarry(USceneComponent* AttachTo, const FTransform& Relative);
	/** Back into the world at Transform, falling under physics until it settles. */
	void Drop(const FTransform& Transform);
	int32 GetCount() const { return Count; }

private:
	void Init(EForestItem InItem, int32 InCount, UStaticMesh* InMesh, bool bInCosmetic, const FVector& Impulse);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleInstanceOnly)
	EForestItem Item = EForestItem::None;

	UPROPERTY(VisibleInstanceOnly)
	int32 Count = 1;

	bool bCosmetic = false;
	float Age = 0.f;
};
