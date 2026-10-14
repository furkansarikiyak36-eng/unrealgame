#pragma once

#include "CoreMinimal.h"
#include "BuildingNodeActor.h"
#include "WorldCore/UndergroundHazardTypes.h"
#include "UndergroundSupportActor.generated.h"

class UStaticMeshComponent;

/**
 * Load-bearing underground support integrated with the existing building-node graph.
 * It does not create a second structural network.
 */
UCLASS(BlueprintType, Blueprintable)
class MYPROJECT2_API AUndergroundSupportActor : public ABuildingNodeActor
{
    GENERATED_BODY()

public:
    AUndergroundSupportActor();

    virtual void BeginPlay() override;
    virtual float ApplyDamage(float Amount, const FVector& Direction) override;
    virtual void TriggerChaosCollapse(const FVector& Impulse = FVector::ZeroVector) override;

    UFUNCTION(BlueprintCallable, Category = "Underground|Support")
    void ConfigureSupport(const FGuid& InSupportId, EUndergroundSupportType InType, float InIntegrity);

    UFUNCTION(BlueprintCallable, Category = "Underground|Support")
    void ApplyMoistureExposure(float WaterVolumeM3);

    UFUNCTION(BlueprintPure, Category = "Underground|Support")
    bool IsSupportOperational() const { return !IsCollapsed() && CurrentIntegrity > 0.0f; }

    UFUNCTION(BlueprintPure, Category = "Underground|Support")
    float GetSupportIntegrity() const { return CurrentIntegrity; }

    UFUNCTION(BlueprintPure, Category = "Underground|Support")
    FGuid GetSupportId() const { return SupportId; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Underground|Support")
    FGuid SupportId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underground|Support")
    EUndergroundSupportType SupportType = EUndergroundSupportType::TimberPost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underground|Support", meta = (ClampMin = "1"))
    float MaximumIntegrity = 100.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Underground|Support")
    float CurrentIntegrity = 100.0f;

    /** Integrity loss per cubic meter of water transferred through/onto the support. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Underground|Support", meta = (ClampMin = "0"))
    float MoistureDamagePerM3 = 18.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Underground|Support")
    TObjectPtr<UStaticMeshComponent> SupportMesh;

protected:
    void ApplySupportProfile();
};
