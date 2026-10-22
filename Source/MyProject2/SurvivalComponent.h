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
/** Persistable state for the existing health/stamina/hunger/thirst owner. */
USTRUCT(BlueprintType)
struct FSurvivalComponentSnapshot
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save")
	int32 SchemaVersion = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save")
	float MaxValue = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save")
	float Health = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save")
	float Stamina = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save")
	float Hunger = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save")
	float Thirst = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save")
	bool bExhausted = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save")
	bool bDead = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save")
	float StaminaIdleTime = 0.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Survival|Save")
	float StarvingTime = 0.0f;

	bool Validate(FString& OutError) const
	{
		OutError.Reset();
		if (SchemaVersion != 1 || !FMath::IsFinite(MaxValue) || MaxValue < 1.0f || MaxValue > 1000000.0f)
		{
			OutError = TEXT("Unsupported survival snapshot schema or max value.");
			return false;
		}
		const auto IsValueInRange = [this](float Value)
		{
			return FMath::IsFinite(Value) && Value >= 0.0f && Value <= MaxValue;
		};
		const auto IsValidTimer = [](float Value)
		{
			return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 100000000.0f;
		};
		if (!IsValueInRange(Health) || !IsValueInRange(Stamina)
			|| !IsValueInRange(Hunger) || !IsValueInRange(Thirst)
			|| !IsValidTimer(StaminaIdleTime) || !IsValidTimer(StarvingTime))
		{
			OutError = TEXT("Survival snapshot contains an invalid stat or timer.");
			return false;
		}
		if (bDead != (Health <= 0.0f))
		{
			OutError = TEXT("Survival snapshot dead flag does not match health.");
			return false;
		}
		return true;
	}
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

	FSurvivalComponentSnapshot CaptureSnapshot() const;
	bool RestoreSnapshot(const FSurvivalComponentSnapshot& Snapshot, FString& OutError);

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
