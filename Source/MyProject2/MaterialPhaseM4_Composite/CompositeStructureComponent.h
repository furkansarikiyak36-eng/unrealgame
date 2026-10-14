// CompositeStructureComponent.h
#pragma once
#include "MaterialCore/MaterialTypes.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StructuralLayer.h"
#include "CompositeStructureComponent.generated.h"

class UStructureLayerComponent;

UCLASS(ClassGroup=(Material), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UCompositeStructureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCompositeStructureComponent();

    UFUNCTION(BlueprintCallable, Category = "Composite")
    UStructureLayerComponent* AddLayer(EStructureLayerRole Role, FName MaterialID, float Thickness);

    UFUNCTION(BlueprintCallable, Category = "Composite")
    UStructureLayerComponent* FindLayer(EStructureLayerRole Role) const;

    UFUNCTION(BlueprintCallable, Category = "Composite")
    void ApplyDamageToLayerRole(EStructureLayerRole Role, float Amount, EMaterialReactionType Reaction);

    UFUNCTION(BlueprintCallable, Category = "Composite")
    void ApplyDamageToAllLayers(float Amount, EMaterialReactionType Reaction);

    UFUNCTION(BlueprintPure, Category = "Composite")
    float GetTotalLoadBearing() const;

    UFUNCTION(BlueprintPure, Category = "Composite")
    bool IsStructureStable() const;

    UFUNCTION(BlueprintPure, Category = "Composite")
    EStructureLayerCondition GetWorstCondition() const;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
    TArray<TObjectPtr<UStructureLayerComponent>> Layers;
};
