#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StickTypes.h"
#include "Core/StructuralMaterialTypes.h"
#include "Core/StructuralNode.h"
#include "StickActor.generated.h"
class UStaticMeshComponent; class UStickStateMachine; class UStickConnectionGraph; class UStickDefinition; class UStickDurabilityComponent; class UStickFlexComponent;
UCLASS()
class MYPROJECT2_API AStickActor : public AActor, public IStructuralNode
{
    GENERATED_BODY()
public:
    AStickActor();
    UFUNCTION(BlueprintCallable) void InitializeFromDefinition(UStickDefinition* Def);
    UFUNCTION(BlueprintPure) UStickDefinition* GetDefinition() const { return Definition; }
    virtual EStructuralMaterialType GetMaterialType_Implementation() const override;
    virtual EStructuralNodeState GetStructuralState_Implementation() const override;
    virtual bool SetStructuralState_Implementation(EStructuralNodeState NewState) override;
    virtual float GetStructuralMass_Implementation() const override;
    virtual float GetStructuralStrength_Implementation() const override;
    virtual bool IsGrounded_Implementation() const override;
    UFUNCTION(BlueprintCallable) void ApplyShape(EStickShape NewShape);
    UFUNCTION(BlueprintCallable) void ApplyDamage(float Amount);
    UFUNCTION(BlueprintPure) float GetHealth() const { return Health; }
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> MeshComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStickStateMachine> StateMachine;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStickConnectionGraph> ConnectionGraph;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStickDurabilityComponent> DurabilityComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStickFlexComponent> FlexComponent;
protected:
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly) TObjectPtr<UStickDefinition> Definition;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float Health = 30.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float MaxHealth = 30.f;
    UFUNCTION() void HandleStateChanged(AActor* Stick, EStructuralNodeState NewState);
};
