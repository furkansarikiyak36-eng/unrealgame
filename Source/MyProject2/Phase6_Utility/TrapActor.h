#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrapActor.generated.h"

class UBoxComponent;
class UTrapMechanismComponent;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class ETrapType : uint8
{
	RollingLog,
	FallingLog,
	SpikePit
};

UCLASS()
class MYPROJECT2_API ATrapActor : public AActor
{
	GENERATED_BODY()

public:
	ATrapActor();

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void Trigger(AActor* Victim);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void Arm();
	UFUNCTION(BlueprintCallable, Category = "Trap")
	void Disarm();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap")
	TObjectPtr<UBoxComponent> TriggerVolume;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap")
	TObjectPtr<UTrapMechanismComponent> Mechanism;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	ETrapType TrapType = ETrapType::RollingLog;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0.0"))
	float Damage = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0.0"))
	float Cooldown = 5.f;

protected:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);
};
