// MaterialInteractionSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MaterialCore/MaterialTypes.h"
#include "MaterialDefinition.h"
#include "InteractionRule.h"
#include "MaterialTypes_Extra.h"

class UMaterialInteractionMatrix;
class UFireMaterialRule;
class UWaterMaterialRule;
class UImpactMaterialRule;
class UTimeMaterialRule;

#include "MaterialInteractionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMaterialDamage, AActor*, Target, float, Damage, EMaterialReactionType, Reaction);

UCLASS()
class MYPROJECT2_API UMaterialInteractionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void ConfigureMatrix(UMaterialInteractionMatrix* NewMatrix) { Matrix = NewMatrix; }

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    float ApplyInteraction(AActor* Target, UMaterialDefinition* Material, EInteractionType Type, const FMaterialInteractionContext& Context);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    EMaterialReactionType PredictReaction(const UMaterialDefinition* Material, EInteractionType Type, const FMaterialInteractionContext& Context) const;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMaterialDamage OnMaterialDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matrix")
    TObjectPtr<UMaterialInteractionMatrix> Matrix;

protected:
    UPROPERTY() TObjectPtr<UFireMaterialRule> FireRule;
    UPROPERTY() TObjectPtr<UWaterMaterialRule> WaterRule;
    UPROPERTY() TObjectPtr<UImpactMaterialRule> ImpactRule;
    UPROPERTY() TObjectPtr<UTimeMaterialRule> TimeRule;
};
