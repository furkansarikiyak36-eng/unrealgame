// MaterialDefinitionLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MaterialDefinition.h"
class UMaterialRegistry;

#include "MaterialDefinitionLibrary.generated.h"

UCLASS(BlueprintType)
class MYPROJECT2_API UMaterialDefinitionLibrary : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
    TArray<TSoftObjectPtr<UMaterialDefinition>> MaterialDefinitions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    TSoftObjectPtr<UMaterialDefinition> DefaultStone;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    TSoftObjectPtr<UMaterialDefinition> DefaultWood;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    TSoftObjectPtr<UMaterialDefinition> DefaultPlaster;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    TSoftObjectPtr<UMaterialDefinition> DefaultSoil;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
    TSoftObjectPtr<UMaterialDefinition> DefaultMetal;

    UFUNCTION(BlueprintCallable, Category = "Load")
    void LoadAllMaterials(UMaterialRegistry* Registry) const;

    UFUNCTION(BlueprintCallable, Category = "Validate")
    bool ValidateLibrary(FString& OutError) const;
};
