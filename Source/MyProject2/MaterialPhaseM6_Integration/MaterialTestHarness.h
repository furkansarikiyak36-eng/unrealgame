// MaterialTestHarness.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MaterialTestHarness.generated.h"

UCLASS()
class MYPROJECT2_API UMaterialTestHarness : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Test")
    static bool TestStoneProperties();

    UFUNCTION(BlueprintCallable, Category = "Test")
    static bool TestWoodProperties();

    UFUNCTION(BlueprintCallable, Category = "Test")
    static bool TestPlasterProperties();

    UFUNCTION(BlueprintCallable, Category = "Test")
    static bool TestFireRule();

    UFUNCTION(BlueprintCallable, Category = "Test")
    static bool TestWaterRule();

    UFUNCTION(BlueprintCallable, Category = "Test")
    static bool TestImpactRule();

    UFUNCTION(BlueprintCallable, Category = "Test")
    static bool TestTimeRule();

    UFUNCTION(BlueprintCallable, Category = "Test")
    static bool TestCompositeStructure();

    UFUNCTION(BlueprintCallable, Category = "Test")
    static bool TestLayerDamagePropagation();

    UFUNCTION(BlueprintCallable, Category = "Test")
    static bool RunAllTests(FString& OutReport);
};
