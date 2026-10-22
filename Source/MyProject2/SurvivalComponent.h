// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivalComponent.generated.h"

/**
 * The four survival values of the GDD (6.1): health, stamina, hunger and thirst.
 * Stamina gates sprinting and heavy actions; hunger and thirst only hurt after
 * staying empty for a while; at zero health the owner dies and respawns.
 */

/** Versioned-by-container survival snapshot, including local environmental exposure. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FSurvivalStateSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save") float Health = 100.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save") float Stamina = 100.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save") float Hunger = 100.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save") float Thirst = 100.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save") float BodyTemperatureC = 37.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save") float AmbientTemperatureC = 18.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save") float Wetness01 = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save") float ShelterHeatC = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save") float StaminaIdleTime = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save") float StarvingTime = 0.0f;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save") bool bExhausted = false;
};

UCLASS(ClassGroup = (Forest), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API USurvivalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float GetHealth() const { return Health; }
	float GetStamina() const { return Stamina; }
	float GetHunger() const { return Hunger; }
	float GetThirst() const { return Thirst; }
	float GetMax() const { return MaxValue; }

	/** Sprinting drains stamina every tick while true. */
	void SetSprinting(bool bInSprinting) { bSprinting = bInSprinting; }
	/** False while exhausted: stamina ran out and has not refilled to the recovery threshold yet. */
	bool CanSprint() const { return !bExhausted && Stamina > 0.f; }

	/** Pays for a heavy action (swing, carry step). Returns false and pays nothing if there is not enough. */
	bool TryUseStamina(float Amount);

	void Eat(float Amount);
	void Drink(float Amount);
	void ApplyDamage(float Amount);
	void ResetAll();

	bool IsStarving() const { return Hunger <= 0.f || Thirst <= 0.f; }

	/** Called once when health reaches zero. */
	TFunction<void()> OnDeath;

	UPROPERTY(EditAnywhere, Category = "Survival")
	float MaxValue = 100.f;

	UPROPERTY(EditAnywhere, Category = "Survival|Stamina")
	float SprintCostPerSecond = 15.f;

	UPROPERTY(EditAnywhere, Category = "Survival|Stamina")
	float StaminaRegenPerSecond = 20.f;

	UPROPERTY(EditAnywhere, Category = "Survival|Stamina")
	float StaminaRegenDelay = 1.f;

	/** After running dry, sprinting stays locked until stamina is back to this much. */
	UPROPERTY(EditAnywhere, Category = "Survival|Stamina")
	float ExhaustionRecovery = 25.f;

	/** Minutes from full to empty. */
	UPROPERTY(EditAnywhere, Category = "Survival|Needs")
	float HungerMinutes = 45.f;

	UPROPERTY(EditAnywhere, Category = "Survival|Needs")
	float ThirstMinutes = 30.f;

	/** Seconds a need may sit at zero before health starts to drop. */
	UPROPERTY(EditAnywhere, Category = "Survival|Needs")
	float StarvationGrace = 60.f;

	UPROPERTY(EditAnywhere, Category = "Survival|Needs")
	float StarvationDamagePerSecond = 0.2f;

	/** Slow natural healing while both needs are above half. */
	UPROPERTY(EditAnywhere, Category = "Survival|Health")
	float HealthRegenPerSecond = 0.1f;

private:
	float Health = 100.f;
	float Stamina = 100.f;
	float Hunger = 100.f;
	float Thirst = 100.f;

	bool bSprinting = false;
	bool bExhausted = false;
	bool bDead = false;
	float StaminaIdleTime = 0.f;
	float StarvingTime = 0.f;
};
