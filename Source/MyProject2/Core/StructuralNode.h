#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/StructuralMaterialTypes.h"
#include "StructuralNode.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UStructuralNode : public UInterface
{
    GENERATED_BODY()
};

class MYPROJECT2_API IStructuralNode
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Structural")
    EStructuralMaterialType GetMaterialType() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Structural")
    EStructuralNodeState GetStructuralState() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Structural")
    bool SetStructuralState(EStructuralNodeState NewState);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Structural")
    float GetStructuralMass() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Structural")
    float GetStructuralStrength() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Structural")
    bool IsGrounded() const;
};
