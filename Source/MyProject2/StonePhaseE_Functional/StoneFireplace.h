#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoneFireplace.generated.h"
class UStaticMeshComponent;
class UPointLightComponent;
class UParticleSystemComponent;

UCLASS()
class MYPROJECT2_API AStoneFireplace : public AActor
{
    GENERATED_BODY()
public:
    AStoneFireplace();
    UFUNCTION(BlueprintCallable) bool AddFuel(float Amount);
    UFUNCTION(BlueprintCallable) bool Ignite();
    UFUNCTION(BlueprintCallable) void Extinguish();
    UFUNCTION(BlueprintCallable) bool PlaceCookingPot();
    UFUNCTION(BlueprintPure) bool IsLit() const { return bIsLit; }
    UFUNCTION(BlueprintPure) float GetHeatOutput() const { return bIsLit ? HeatOutput : 0.0f; }
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FireplaceMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> FireLight;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UParticleSystemComponent> FireParticles;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxFuel = 100.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float CurrentFuel = 0.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) bool bIsLit = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float HeatOutput = 500.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHasCookingPot = false;
protected:
    virtual void Tick(float DeltaTime) override;
};
