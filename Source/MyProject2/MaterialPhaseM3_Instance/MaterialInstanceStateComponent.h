// MaterialInstanceStateComponent.h
#pragma once
#include "MaterialCore/MaterialTypes.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MaterialInstanceState.h"
#include "MaterialPhaseM2_Interaction/InteractionRule.h"
#include "MaterialInstanceStateComponent.generated.h"

class UMaterialDefinition;

UCLASS(ClassGroup=(Material), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UMaterialInstanceStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMaterialInstanceStateComponent();

    UFUNCTION(BlueprintCallable, Category = "State")
    void InitializeState(UMaterialDefinition* Def);

    UFUNCTION(BlueprintCallable, Category = "State")
    void ApplyDamage(float Amount, EMaterialReactionType Reaction);

    UFUNCTION(BlueprintCallable, Category = "State")
    void UpdateFromInteraction(const FMaterialInteractionContext& Context, float Damage);

    UFUNCTION(BlueprintPure, Category = "State")
    const FMaterialInstanceState& GetState() const { return State; }

    UFUNCTION(BlueprintPure, Category = "State")
    UMaterialDefinition* GetDefinition() const { return Definition; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    TObjectPtr<UMaterialDefinition> Definition;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
    FMaterialInstanceState State;

};
