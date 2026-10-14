// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickStructure.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

/** What sticks are made into (Sons of the Forest's stick builds). */
UENUM()
enum class EStickBuild : uint8
{
	/** A row of sharpened sticks driven into the ground, points out: hurts what walks into it. */
	Spikes,
	/** A woven stick fence: blocks the way until it is broken. */
	Fence,
	/** A stick torch on a stake: light and a little warmth. */
	Torch,
	Count UMETA(Hidden),
};

/**
 * A small build made of sticks from the pack: spikes, a fence or a torch. It has a strength like wood (0 sound, 1 broken);
 * raiders hitting it wear it down.
 */
UCLASS()
class MYPROJECT2_API AStickStructure : public AActor
{
	GENERATED_BODY()

public:
	AStickStructure();

	static int32 SticksFor(EStickBuild Build);
	static FString NameOf(EStickBuild Build);
	/** A build standing on the ground at Ground, its length across Yaw. */
	static AStickStructure* Build(UWorld* World, EStickBuild Build, const FVector& Ground, float Yaw);

	EStickBuild GetKind() const { return Kind; }
	float GetDamage() const { return Damage; }
	/** Wears it down; it falls apart (is destroyed) at 1. */
	void AddDamage(float Amount);

	/** Spikes hurt enemies this close (cm) this much per second; a torch warms this far. */
	static constexpr float SpikeReachCm = 70.f;
	static constexpr float SpikeDps = 20.f;
	static constexpr float TorchReachCm = 300.f;
	static constexpr float TorchWarmthC = 12.f;
	bool IsLitTorch() const { return Kind == EStickBuild::Torch; }

private:
	void Configure(EStickBuild NewKind);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> Light;

	EStickBuild Kind = EStickBuild::Spikes;
	float Damage = 0.f;
};
