// MaterialValidation.h
#pragma once
#include "MaterialCore/MaterialTypes.h"

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MaterialDefinition.h"
#include "MaterialValidation.generated.h"

UCLASS()
class MYPROJECT2_API UMaterialValidation : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Validation")
    static bool ValidateDefinition(UMaterialDefinition* Def, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "Validation")
    static bool ValidateAgainstTemplate(UMaterialDefinition* Def, EMaterialClass Template, FString& OutError);
};
