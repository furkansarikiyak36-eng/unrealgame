#pragma once
#include "CoreMinimal.h"
#include "InventoryCore/InventoryCategory.h"
#include "CraftingTypes.generated.h"
UENUM(BlueprintType) enum class ECraftingConditionType:uint8{None,NearFire,NearWater,NearWorkbench,TimeOfDay,WeatherClear};
USTRUCT(BlueprintType) struct FInventoryCraftingIngredient
{GENERATED_BODY() UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ItemID=NAME_None; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Count=1; UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bConsumed=true; UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bIsTool=false; UPROPERTY(EditAnywhere,BlueprintReadWrite) FName RequiredTag=NAME_None;};
USTRUCT(BlueprintType) struct FCraftingCondition{GENERATED_BODY() UPROPERTY(EditAnywhere,BlueprintReadWrite) ECraftingConditionType Type=ECraftingConditionType::None; UPROPERTY(EditAnywhere,BlueprintReadWrite) float Value=0; UPROPERTY(EditAnywhere,BlueprintReadWrite) FName Tag=NAME_None;};
USTRUCT(BlueprintType) struct FCraftingOutput{GENERATED_BODY() UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ItemID=NAME_None; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Count=1; UPROPERTY(EditAnywhere,BlueprintReadWrite) float CraftTime=2.0f;};
USTRUCT(BlueprintType) struct FCraftingResult{GENERATED_BODY() UPROPERTY(BlueprintReadWrite) EInventoryResult Result=EInventoryResult::Success; UPROPERTY(BlueprintReadWrite) FName OutputItemID=NAME_None; UPROPERTY(BlueprintReadWrite) int32 OutputCount=0; UPROPERTY(BlueprintReadWrite) FText ErrorMessage;};
