// MaterialTypes_Extra.h (EInteractionType için)
#pragma once

#include "CoreMinimal.h"
#include "MaterialTypes_Extra.generated.h"

UENUM(BlueprintType)
enum class EInteractionType : uint8
{
    None        UMETA(DisplayName = "Yok"),
    Impact      UMETA(DisplayName = "Darbe"),
    Fire        UMETA(DisplayName = "Ateş"),
    Water       UMETA(DisplayName = "Su"),
    Time        UMETA(DisplayName = "Zaman"),
    Chemical    UMETA(DisplayName = "Kimyasal"),
    Structural  UMETA(DisplayName = "Yapısal")
};
