// CorrosionComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CorrosionComponent.generated.h"

class UMetalDefinition;

UCLASS(ClassGroup=(Material), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UCorrosionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCorrosionComponent();

    UFUNCTION(BlueprintCallable, Category = "Corrosion")
    void InitializeFromDefinition(UMetalDefinition* Def);

    UFUNCTION(BlueprintCallable, Category = "Corrosion")
    void UpdateCorrosion(float Moisture, float DeltaDays);

    UFUNCTION(BlueprintCallable, Category = "Corrosion")
    void ApplyAntiCorrosionTreatment(float Amount);

    UFUNCTION(BlueprintPure, Category = "Corrosion")
    float GetCorrosionLevel() const { return CorrosionLevel; }

    UFUNCTION(BlueprintPure, Category = "Corrosion")
    bool IsWeakened() const { return CorrosionLevel > 0.5f; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    TObjectPtr<UMetalDefinition> Definition;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
    float CorrosionLevel = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
    float ProtectionLevel = 0.0f;
};
