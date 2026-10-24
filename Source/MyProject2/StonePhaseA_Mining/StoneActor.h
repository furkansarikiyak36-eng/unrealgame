#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoneTypes.h"
#include "Core/StructuralMaterialTypes.h"
#include "Core/StructuralNode.h"
#include "StoneActor.generated.h"

class UStaticMeshComponent;
class UStoneStateMachine;
class UStoneConnectionGraph;
class UStoneDefinition;

UCLASS()
class MYPROJECT2_API AStoneActor : public AActor, public IStructuralNode
{
    GENERATED_BODY()
public:
    AStoneActor();
    UFUNCTION(BlueprintCallable) void InitializeFromDefinition(UStoneDefinition* Def);
    UFUNCTION(BlueprintPure) UStoneDefinition* GetDefinition() const { return Definition; }
    UFUNCTION(BlueprintPure) UStoneDefinition* GetBaseDefinition() const { return BaseDefinition ? BaseDefinition : Definition; }
    bool RestorePersistentState(UStoneDefinition* SavedBaseDefinition, EStoneShape SavedShape,
        const FVector& SavedDimensions, float SavedHealth, EStructuralNodeState SavedState, FString& OutError);
    virtual EMaterialType GetMaterialType_Implementation() const override;
    virtual EStructuralNodeState GetStructuralState_Implementation() const override;
    virtual bool SetStructuralState_Implementation(EStructuralNodeState NewState) override;
    virtual float GetStructuralMass_Implementation() const override;
    virtual float GetStructuralStrength_Implementation() const override;
    virtual bool IsGrounded_Implementation() const override;
    UFUNCTION(BlueprintCallable) void ApplyShape(EStoneShape NewShape, FVector NewDimensions);
    UFUNCTION(BlueprintCallable) void ApplyDamage(float Amount);
    UFUNCTION(BlueprintPure) float GetHealth() const { return Health; }
    UFUNCTION(BlueprintPure) float GetMaxHealth() const { return MaxHealth; }
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> MeshComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStoneStateMachine> StateMachine;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStoneConnectionGraph> ConnectionGraph;
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Definition") TObjectPtr<UStoneDefinition> Definition;
    /** Original asset definition retained when Definition is duplicated for a runtime-shaped stone. */
    UPROPERTY(Transient) TObjectPtr<UStoneDefinition> BaseDefinition;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float Health = 100.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float MaxHealth = 100.0f;
    UFUNCTION() void HandleStateChanged(AActor* Stone, EStructuralNodeState NewState);
};
