#pragma once
#include "CoreMinimal.h"
#include "RaidTypes.h"
#include "HordeCompositionPicker.generated.h"
UCLASS() class MYPROJECT2_API UHordeCompositionPicker : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) static FName PickWeightedEnemy(const TArray<FWeightedEntry>& Pool); UFUNCTION(BlueprintCallable) static FName PickConditionalEnemy(const TArray<FConditionalEntry>& Pool,int32 StageIndex,FName ContextTag); };
