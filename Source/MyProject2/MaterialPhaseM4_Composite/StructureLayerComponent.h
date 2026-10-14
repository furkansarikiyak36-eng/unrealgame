// StructureLayerComponent.h
#pragma once
#include "MaterialCore/MaterialTypes.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StructuralLayer.h"
#include "StructureLayerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLayerCondition, EStructureLayerRole, Role, EStructureLayerCondition, NewCondition);

UCLASS(ClassGroup=(Material), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStructureLayerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStructureLayerComponent();

    UFUNCTION(BlueprintCallable, Category = "Layer")
    void InitializeLayer(EStructureLayerRole Role, FName MaterialID, float Thickness);

    UFUNCTION(BlueprintCallable, Category = "Layer")
    void ApplyDamageToLayer(float Amount, EMaterialReactionType Reaction);

    UFUNCTION(BlueprintPure, Category = "Layer")
    const FStructuralLayer& GetLayer() const { return Layer; }

    UFUNCTION(BlueprintPure, Category = "Layer")
    EStructureLayerCondition GetCondition() const { return Layer.Condition; }

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnLayerCondition OnLayerConditionChanged;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
    FStructuralLayer Layer;

protected:
    EStructureLayerCondition LastCondition = EStructureLayerCondition::Pristine;
};
