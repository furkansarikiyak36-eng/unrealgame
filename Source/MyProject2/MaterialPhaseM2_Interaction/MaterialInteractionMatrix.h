// MaterialInteractionMatrix.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MaterialCore/MaterialTypes.h"
#include "InteractionRule.h"

class UFireMaterialRule;
class UWaterMaterialRule;
class UImpactMaterialRule;
class UTimeMaterialRule;

#include "MaterialInteractionMatrix.generated.h"

UCLASS(BlueprintType)
class MYPROJECT2_API UMaterialInteractionMatrix : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
    TArray<TObjectPtr<UInteractionRule>> Rules;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    TObjectPtr<UFireMaterialRule> DefaultFireRule;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    TObjectPtr<UWaterMaterialRule> DefaultWaterRule;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    TObjectPtr<UImpactMaterialRule> DefaultImpactRule;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    TObjectPtr<UTimeMaterialRule> DefaultTimeRule;

    UFUNCTION(BlueprintCallable, Category = "Matrix")
    TArray<UInteractionRule*> GetRulesFor(EMaterialClass Class) const;
};
