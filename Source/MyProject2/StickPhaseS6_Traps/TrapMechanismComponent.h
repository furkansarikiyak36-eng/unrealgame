#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TrapMechanismComponent.generated.h"

UENUM(BlueprintType)
enum class ETrapTriggerType : uint8
{
	Pressure,
	TripWire,
	Proximity,
	Manual
};

UENUM(BlueprintType)
enum class ETrapLifecycleState : uint8
{
	Disarmed,
	Armed,
	Triggered,
	Cooldown,
	Consumed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrapTriggered, AActor*, Victim);

UCLASS(ClassGroup = (Trap), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UTrapMechanismComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTrapMechanismComponent();

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void Arm();
	UFUNCTION(BlueprintCallable, Category = "Trap")
	void Disarm();
	UFUNCTION(BlueprintCallable, Category = "Trap")
	bool Trigger(AActor* Victim);

	UFUNCTION(BlueprintPure, Category = "Trap")
	bool IsArmed() const { return bArmed; }
	UFUNCTION(BlueprintPure, Category = "Trap")
	ETrapLifecycleState GetLifecycleState() const { return LifecycleState; }

	UPROPERTY(BlueprintAssignable, Category = "Trap")
	FOnTrapTriggered OnTrapTriggered;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	ETrapTriggerType TriggerType = ETrapTriggerType::Pressure;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0.0"))
	float Cooldown = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0.0"))
	float Damage = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	bool bConsumeOnTrigger = false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Trap")
	bool bArmed = true;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Trap")
	float LastTriggerTime = -100.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Trap")
	ETrapLifecycleState LifecycleState = ETrapLifecycleState::Armed;

private:
	void FinishCooldown();
	FTimerHandle CooldownTimer;
};
