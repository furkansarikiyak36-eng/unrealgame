// PlasterComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlasterDefinition.h"
#include "PlasterComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlasterConditionChanged, float, DamagePercent);

UCLASS(ClassGroup=(Material), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UPlasterComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPlasterComponent();

    UFUNCTION(BlueprintCallable, Category = "Plaster")
    void ApplyPlaster(UPlasterDefinition* Def, float Thickness);

    UFUNCTION(BlueprintCallable, Category = "Plaster")
    void Cure(float DeltaHours);

    UFUNCTION(BlueprintCallable, Category = "Plaster")
    void ApplyWaterDamage(float Intensity, float DurationHours);

    UFUNCTION(BlueprintCallable, Category = "Plaster")
    void ApplyImpactDamage(float Magnitude);

    UFUNCTION(BlueprintPure, Category = "Plaster")
    float GetDamagePercent() const { return DamagePercent; }

    UFUNCTION(BlueprintPure, Category = "Plaster")
    bool IsFullyCured() const { return bIsCured; }

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPlasterConditionChanged OnPlasterConditionChanged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    TObjectPtr<UPlasterDefinition> Definition;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
    float DamagePercent = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
    float AppliedThickness = 0.05f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
    float CureProgress = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
    bool bIsCured = false;
};
