// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestTypes.h"
#include "RawMaterialNode.generated.h"

class AForestCharacter;
class UStaticMeshComponent;

/** What a raw material source in the world is; it decides the look and the default drops. */
UENUM(BlueprintType)
enum class ERawSourceKind : uint8
{
	/** Plant fibre, sometimes a vine. */
	FiberBush UMETA(DisplayName = "Lif Çalısı"),
	/** Vines for lashing joints. */
	Vines UMETA(DisplayName = "Asma"),
	/** A dead standing tree: bark, sometimes resin. */
	DeadTree UMETA(DisplayName = "Kuru Ağaç"),
	/** A river bank of clay for sealing the gaps between logs. */
	ClayBank UMETA(DisplayName = "Kil Yatağı"),
	/** Flint knocked off an outcrop. */
	FlintOutcrop UMETA(DisplayName = "Çakmak Taşı Damarı"),
	/** Bones of an old kill. */
	BonePile UMETA(DisplayName = "Kemik Yığını")
};

USTRUCT(BlueprintType)
struct FRawMaterialDrop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	EForestItem Item = EForestItem::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop", meta = (ClampMin = "0"))
	int32 Min = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop", meta = (ClampMin = "0"))
	int32 Max = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop", meta = (ClampMin = "0", ClampMax = "1"))
	float Probability = 1.f;
};

/**
 * A source of raw materials gathered by hand with [E]: fibre from a bush, clay from a bank, flint from an outcrop. Raw
 * materials are not building pieces; they bind and seal the pieces the player puts together (rope, lashing, chinking).
 * Each gathering takes one from the supply; an empty source regrows after RespawnSeconds.
 * Placed by the scenario, or dragged into a level: Kind fills in the look and, when Drops is empty, the default drops.
 */
UCLASS()
class MYPROJECT2_API ARawMaterialNode : public AActor
{
	GENERATED_BODY()

public:
	ARawMaterialNode();

	static ARawMaterialNode* Spawn(UWorld* World, const FVector& Ground, ERawSourceKind Kind);
	/** The drops a source of this kind gives unless set by hand. */
	static TArray<FRawMaterialDrop> DefaultDrops(ERawSourceKind Kind);
	static FString KindName(ERawSourceKind Kind);

	/** One gathering into the player's pack. Returns how many items went in; 0 when empty or the pack is full. */
	int32 Harvest(AForestCharacter* Player, FString& OutGot);
	/** Back to a full supply (the respawn timer, or the mechanics test). */
	void Regrow();

	bool IsDepleted() const { return MaxSupply > 0 && Supply <= 0; }
	/** 1 when full, 0 when used up; always 1 for an endless source. */
	float GetSupplyRatio() const { return MaxSupply > 0 ? float(Supply) / float(MaxSupply) : 1.f; }
	float GetRegrowSecondsLeft() const;
	/** "[E] Topla: Bitki Lifi" text for the HUD. */
	FString GetPrompt() const;

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raw Material")
	ERawSourceKind Kind = ERawSourceKind::FiberBush;

	/** Left empty, the kind's default drops are used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raw Material")
	TArray<FRawMaterialDrop> Drops;

	/** Gatherings before the source is empty; 0 = endless. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raw Material", meta = (ClampMin = "0"))
	int32 MaxSupply = 3;

	/** Seconds an empty source takes to regrow; 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raw Material", meta = (ClampMin = "0"))
	float RespawnSeconds = 90.f;

protected:
	virtual void BeginPlay() override;

private:
	void ApplyKind();
	void UpdateLook();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	int32 Supply = 0;
	FVector FullScale = FVector::OneVector;
	FLinearColor Color = FLinearColor::White;
	FTimerHandle RegrowTimer;
};
