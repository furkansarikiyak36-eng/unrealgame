// MaterialRegistry.h
#pragma once
#include "MaterialCore/MaterialTypes.h"

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MaterialDefinition.h"
#include "MaterialRegistry.generated.h"

UCLASS()
class MYPROJECT2_API UMaterialRegistry : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Registry")
    void RegisterMaterial(UMaterialDefinition* Definition);

    UFUNCTION(BlueprintCallable, Category = "Registry")
    UMaterialDefinition* FindMaterial(FName MaterialID) const;

    UFUNCTION(BlueprintCallable, Category = "Registry")
    TArray<UMaterialDefinition*> GetAllMaterials() const;

    UFUNCTION(BlueprintCallable, Category = "Registry")
    TArray<UMaterialDefinition*> FindMaterialsByClass(EMaterialClass Class) const;

    UFUNCTION(BlueprintCallable, Category = "Registry")
    int32 GetRegisteredCount() const { return Materials.Num(); }

protected:
    UPROPERTY() TMap<FName, TObjectPtr<UMaterialDefinition>> Materials;
};
