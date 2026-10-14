// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HaulCart.generated.h"

class ACarvableLog;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EHaulKind : uint8
{
	/** A hand cart: two wheels, carries two shoulder loads, rolls easily. */
	Cart,
	/** A log sled: skids on the ground, carries four shoulder loads, slides less easily. */
	Sled,
	/** A storage rack: stands where it is, holds eight shoulder loads. */
	Rack,
};

/**
 * Something wood is loaded on: a cart, a sled or a rack. Pieces keep their volume on it (a load never exceeds the
 * capacity in wood) and are Carried while on it; unloading sets them down Loose beside it.
 */
UCLASS()
class MYPROJECT2_API AHaulCart : public AActor
{
	GENERATED_BODY()

public:
	AHaulCart();

	/** A cart, sled or rack standing on Ground, facing Yaw. */
	static AHaulCart* Spawn(UWorld* World, const FVector& Ground, float Yaw, EHaulKind Kind);

	static FString KindName(EHaulKind Kind);
	EHaulKind GetKind() const { return Kind; }

	/** Wood the thing holds (cm3): this many shoulder loads of one whole log each. */
	/** Selects a cart/sled/rack variant only while it has no cargo. */
	UFUNCTION(BlueprintCallable, Category = "Hauling")
	void SetKind(EHaulKind NewKind);

	float GetCapacity() const;
	float GetLoadVolume() const;
	int32 GetLoadCount() const { return Loaded.Num(); }
	bool CanLoad(const ACarvableLog* Piece) const;

	/** Puts the piece on the load (off the shoulder). False if it does not fit. */
	bool Load(ACarvableLog* Piece);
	/** Takes the last piece off and sets it down loose beside the cart; null if empty. */
	ACarvableLog* Unload();

	/** A push: carts and sleds glide a while and stop; a rack does not move. */
	bool Shove(const FVector& Velocity);

	virtual void Tick(float DeltaSeconds) override;

private:
	void Configure(EHaulKind NewKind);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Bed;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	UPROPERTY()
	TArray<TWeakObjectPtr<ACarvableLog>> Loaded;

	EHaulKind Kind = EHaulKind::Cart;
	/** Height of the bed's top above the ground. */
	float BedTop = 45.f;
	FVector Velocity = FVector::ZeroVector;
};
