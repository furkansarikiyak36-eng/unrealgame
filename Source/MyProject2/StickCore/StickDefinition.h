#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StickTypes.h"
class UMaterialDefinition;
#include "StickDefinition.generated.h"

UCLASS(BlueprintType)
class MYPROJECT2_API UStickDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") FName StickName;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Material") TSoftObjectPtr<UMaterialDefinition> CommonMaterialDefinition;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") EStickShape Shape = EStickShape::Raw;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") EStickSize Size = EStickSize::Medium;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") EStickRole Role = EStickRole::Crafting;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimensions") float Length = 80.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimensions") float Diameter = 3.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Physics") float Mass = 0.4f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Physics") float Volume = 0.0006f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Physics") float Buoyancy = 0.9f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Physics") float Flexibility = 0.3f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Structural") float BendingStrength = 15.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Structural") float MaxCantilever = 80.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fire") float Flammability = 0.9f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fire") float BurnTime = 30.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fire") float WetnessResistance = 0.4f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage") float Durability = 30.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stacking") int32 MaxStackSize = 20;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mesh") TSoftObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mesh") TSoftObjectPtr<UMaterialInterface> Material;
    UFUNCTION(BlueprintCallable, Category="Validation") bool ValidateDefinition(FString& OutError) const;
    UFUNCTION(BlueprintCallable, Category="Derived") float GetVolumeFromDimensions() const;
    UFUNCTION(BlueprintPure, Category="Derived") bool IsWet() const { return Shape == EStickShape::Wet; }
    UFUNCTION(BlueprintCallable, Category="Derived") float GetEffectiveBurnTime() const;
};
