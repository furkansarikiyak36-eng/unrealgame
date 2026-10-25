#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StakeActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UPrimitiveComponent;

UCLASS()
class MYPROJECT2_API AStakeActor : public AActor
{
	GENERATED_BODY()

public:
	AStakeActor();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Defense")
	void Sharpen();

	UFUNCTION(BlueprintCallable, Category = "Defense")
	void ApplyDamageToTarget(AActor* Target, float DamageAmount);

	bool ApplyStoneReinforcement(int32 StoneCount, float StrengthBonusPerStone);
	UFUNCTION(BlueprintCallable, Category = "Defense")
	void ApplyStructuralDamage(float Amount);

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Defense")
	float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "Defense")
	float GetStrengthMultiplier() const { return StructuralStrengthMultiplier; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> StakeMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> DamageSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense", meta = (ClampMin = "0.0"))
	float Damage = 35.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Defense")
	float Health = 100.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Defense")
	bool bSharpened = false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Defense")
	bool bReinforced = false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Defense")
	int32 ReinforcementStoneCount = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Defense")
	float StructuralStrengthMultiplier = 1.f;

protected:
	UFUNCTION()
	void HandleDamageOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

private:
	void UpdateDamageVolume();
	UPROPERTY(VisibleInstanceOnly, Category = "Defense")
	TArray<TObjectPtr<UStaticMeshComponent>> ReinforcementStones;
};
