// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestTypes.h"
#include "StructureManager.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Holds every piece the player places. Each item type gets one instanced mesh,
 * so a whole wall or log cabin costs a handful of draw calls.
 */
UCLASS()
class MYPROJECT2_API AStructureManager : public AActor
{
	GENERATED_BODY()

public:
	AStructureManager();

	void AddPiece(EForestItem Item, const FTransform& WorldTransform);

	/** Placed pieces of one kind, or null if none were placed yet. */
	const UInstancedStaticMeshComponent* FindPieces(EForestItem Item) const;

private:
	UInstancedStaticMeshComponent* GetOrCreatePieces(EForestItem Item);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TMap<EForestItem, TObjectPtr<UInstancedStaticMeshComponent>> Pieces;
};
