#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StoneTypes.h"
#include "StoneDefinition.generated.h"
class UStaticMesh;
class UMaterialInterface;
class UMaterialDefinition;

UCLASS(BlueprintType)
class MYPROJECT2_API UStoneDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity") FName StoneName;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Material") TSoftObjectPtr<UMaterialDefinition> CommonMaterialDefinition;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity") EStoneType Type = EStoneType::Granite;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity") EStoneShape Shape = EStoneShape::Building;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity") EStoneRole Role = EStoneRole::None;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dimensions") FVector Dimensions = FVector(30, 20, 20);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics") float Mass = 25.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics") float Volume = 0.012f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics") float StaticFriction = 0.7f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics") float RollingResistance = 0.8f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structural") float CompressiveStrength = 500.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structural") float TensileStrength = 30.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structural") float MaxStackHeight = 20.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage") float FractureThreshold = 0.3f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage") float WeatheringRate = 0.05f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire") float HeatConductivity = 0.8f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Faces") TArray<FStoneFace> Faces;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets") TArray<FStoneSocket> Sockets;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh") TSoftObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh") TSoftObjectPtr<UMaterialInterface> Material;

    UFUNCTION(BlueprintCallable, Category = "Validation") bool ValidateDefinition(FString& OutError) const;
    UFUNCTION(BlueprintCallable, Category = "Derived") float GetVolumeFromDimensions() const;
    UFUNCTION(BlueprintCallable, Category = "Derived") bool CanStackOn(const UStoneDefinition* Below) const;
};
