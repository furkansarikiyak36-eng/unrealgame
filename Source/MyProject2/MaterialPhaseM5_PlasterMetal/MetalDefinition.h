// MetalDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "MaterialCore/MaterialDefinition.h"
#include "MetalDefinition.generated.h"

UENUM(BlueprintType)
enum class EMetalType : uint8
{
    Iron        UMETA(DisplayName = "Demir"),
    Steel       UMETA(DisplayName = "Çelik"),
    Copper      UMETA(DisplayName = "Bakır"),
    Bronze      UMETA(DisplayName = "Bronz"),
    Nail        UMETA(DisplayName = "Çivi")
};

UCLASS(BlueprintType)
class MYPROJECT2_API UMetalDefinition : public UMaterialDefinition
{
    GENERATED_BODY()

public:
    UMetalDefinition();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metal")
    EMetalType MetalType = EMetalType::Iron;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metal")
    float CorrosionRate = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metal")
    float FatigueThreshold = 0.7f;
};
