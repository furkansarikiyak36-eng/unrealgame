#pragma once
#include "Core/StructuralMaterialTypes.h"
#include "CoreMinimal.h"
#include "MaterialCore/MaterialTypes.h"
#include "StickTypes.generated.h"

UENUM(BlueprintType)
enum class EStickShape : uint8 { Raw, Broken, Sharpened, DoublePoint, Bundle, Woven, Wet, Dry, Charred };
UENUM(BlueprintType)
enum class EStickSize : uint8 { Tiny, Short, Medium, Long, Branch, Forked };
UENUM(BlueprintType)
enum class EStickRole : uint8 { None, Crafting, Weapon, Tool, Fuel, Light, Frame, Fence, Trap, Furniture, Decoration };

USTRUCT(BlueprintType)
struct FStickBundle
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName StickType;
    UPROPERTY(BlueprintReadWrite) int32 Count = 0;
    UPROPERTY(BlueprintReadWrite) int32 MaxCount = 20;
    UPROPERTY(BlueprintReadWrite) float TotalMass = 0.0f;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStickStateChanged, AActor*, Stick, EStructuralNodeState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStickBroken, AActor*, Stick);
