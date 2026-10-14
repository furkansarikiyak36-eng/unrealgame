#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WorldTypes.h"
#include "WorldEventResponder.generated.h"
UINTERFACE(MinimalAPI,Blueprintable)
class UWorldEventResponder : public UInterface { GENERATED_BODY() };
class MYPROJECT2_API IWorldEventResponder
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category="Event") bool HandleWorldEvent(const FWorldEvent& Event);
};
