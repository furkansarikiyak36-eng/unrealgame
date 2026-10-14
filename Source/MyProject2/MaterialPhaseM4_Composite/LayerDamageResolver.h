// LayerDamageResolver.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MaterialCore/MaterialTypes.h"
#include "LayerDamageResolver.generated.h"

class UCompositeStructureComponent;
class UMaterialDefinition;

UCLASS()
class MYPROJECT2_API ULayerDamageResolver : public UObject
{
    GENERATED_BODY()

public:
    // Kaplama hasarı → iskelet korunması
    UFUNCTION(BlueprintCallable, Category = "Resolver")
    static EStructureLayerRole ResolveDamageTarget(EMaterialReactionType Reaction);

    UFUNCTION(BlueprintCallable, Category = "Resolver")
    static bool ShouldDamagePropagateToSkeleton(EStructureLayerCondition CladdingCondition);

    UFUNCTION(BlueprintCallable, Category = "Resolver")
    static float ComputeLoadOnFoundation(UCompositeStructureComponent* Structure);
};
