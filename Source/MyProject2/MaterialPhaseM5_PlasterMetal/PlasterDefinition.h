// PlasterDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "MaterialCore/MaterialDefinition.h"
#include "PlasterDefinition.generated.h"

UENUM(BlueprintType)
enum class EPlasterType : uint8
{
    Gypsum      UMETA(DisplayName = "Alçı"),
    Lime        UMETA(DisplayName = "Kireç"),
    Clay        UMETA(DisplayName = "Kil"),
    Cement      UMETA(DisplayName = "Çimento")
};

UCLASS(BlueprintType)
class MYPROJECT2_API UPlasterDefinition : public UMaterialDefinition
{
    GENERATED_BODY()

public:
    UPlasterDefinition();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plaster")
    EPlasterType PlasterType = EPlasterType::Gypsum;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plaster")
    float CureTimeHours = 24.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plaster")
    float AdhesionStrength = 50.0f;
};
