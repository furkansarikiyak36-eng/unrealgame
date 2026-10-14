#pragma once
#include "CoreMinimal.h"
#include "RaidTypes.h"
#include "HordeGroupResolver.generated.h"
class UEnemyGroupLibrary;
UCLASS() class MYPROJECT2_API UHordeGroupResolver : public UObject { GENERATED_BODY() public: UFUNCTION(BlueprintCallable) static TArray<FWeightedEntry> ResolveGroup(FName GroupID,const UEnemyGroupLibrary* Library); };
