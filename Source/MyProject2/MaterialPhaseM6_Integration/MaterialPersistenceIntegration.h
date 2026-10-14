// MaterialPersistenceIntegration.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MaterialPhaseM3_Instance/MaterialInstanceState.h"
#include "MaterialPersistenceIntegration.generated.h"

UCLASS()
class MYPROJECT2_API UMaterialPersistenceIntegration : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Persistence")
    static TArray<uint8> SerializeInstanceState(const FMaterialInstanceState& State);

    UFUNCTION(BlueprintCallable, Category = "Persistence")
    static FMaterialInstanceState DeserializeInstanceState(const TArray<uint8>& Bytes);
};
