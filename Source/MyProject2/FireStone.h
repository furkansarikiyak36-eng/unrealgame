// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireStone.generated.h"

class ACampfire;
class UStaticMeshComponent;

/**
 * One stone laid on the ground for a fire pit. Seven stones close together (a ring) become a fire pit; until then each
 * is just a stone lying where it was put.
 */
UCLASS()
class MYPROJECT2_API AFireStone : public AActor
{
	GENERATED_BODY()

public:
	AFireStone();

	static constexpr int32 StonesPerPit = 7;
	/** Stones this close to the newest one (cm) count towards the same ring. */
	static constexpr float RingReachCm = 110.f;

	/** A stone on the ground at Ground. */
	static AFireStone* Place(UWorld* World, const FVector& Ground);

	/** Stones lying within ring reach of Where. */
	static int32 CountNear(UWorld* World, const FVector& Where);

	/**
	 * If StonesPerPit stones lie together around New, they are taken up and a fire pit is built in the middle of them.
	 * Null while the ring is not complete.
	 */
	static ACampfire* TryForm(UWorld* World, const AFireStone* New);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;
};
