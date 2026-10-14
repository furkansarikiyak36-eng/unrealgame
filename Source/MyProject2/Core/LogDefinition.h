#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LogTypes.h"
class UStaticMesh;
class UMaterialInterface;
class UMaterialDefinition;

#include "LogDefinition.generated.h"

UCLASS(BlueprintType)
class MYPROJECT2_API ULogDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") FName LogType = NAME_None;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Material") TSoftObjectPtr<UMaterialDefinition> CommonMaterialDefinition;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") EWoodSpecies Species = EWoodSpecies::Pine;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") EWoodShape Shape = EWoodShape::Full;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimensions", meta=(ClampMin="0.1")) float Length = 65.0f; // cm
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimensions", meta=(ClampMin="0.1")) float Diameter = 20.0f; // cm
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Physics", meta=(ClampMin="0.01")) float Mass = 40.0f; // kg
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Physics", meta=(ClampMin="0.0")) float Volume = 0.02f; // m^3
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Physics", meta=(ClampMin="0.0", ClampMax="1.0")) float Buoyancy = 0.6f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Physics", meta=(ClampMin="0.0")) float RollingResistance = 0.4f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Physics", meta=(ClampMin="0.0")) float DragResistance = 1.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Structural", meta=(ClampMin="0.0")) float StructuralStrength = 100.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Structural", meta=(ClampMin="0.0")) float MaxCantilever = 250.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fire", meta=(ClampMin="0.0", ClampMax="1.0")) float Flammability = 0.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fire", meta=(ClampMin="0.1")) float BurnTime = 120.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sockets") TArray<FLogSocket> DefaultSockets;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mesh") TSoftObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mesh") TSoftObjectPtr<UMaterialInterface> Material;

    UFUNCTION(BlueprintCallable, Category="Validation") bool ValidateDefinition(FString& OutError) const;
    UFUNCTION(BlueprintPure, Category="Derived") float GetVolumeFromDimensions() const;
};
