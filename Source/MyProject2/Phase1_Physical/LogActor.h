#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LogTypes.h"
#include "Core/StructuralMaterialTypes.h"
#include "Core/StructuralNode.h"
#include "LogActor.generated.h"
class UStaticMeshComponent; class ULogStateMachine; class ULogConnectionGraph; class ULogPhysicsComponent; class ULogDefinition; class UStructuralGraph;

UCLASS()
class MYPROJECT2_API ALogActor : public AActor, public IStructuralNode
{
    GENERATED_BODY()
public:
    ALogActor();
    virtual void BeginPlay() override;
    UFUNCTION(BlueprintCallable) void InitializeFromDefinition(ULogDefinition* Def);
    UFUNCTION(BlueprintPure) ULogDefinition* GetDefinition() const { return Definition; }
    UFUNCTION(BlueprintPure) bool IsLogGrounded() const;
    UFUNCTION(BlueprintPure) ELogState GetLogState() const;
    UFUNCTION(BlueprintCallable) bool SetLogState(ELogState NewState);
    UFUNCTION(BlueprintPure) FVector GetCylinderAxis() const;
    UFUNCTION(BlueprintCallable) void ApplyCut(EWoodShape NewShape, float NewLength);
    UFUNCTION(BlueprintPure) float GetCurrentLength() const { return CurrentLength; }
    UFUNCTION(BlueprintPure) EWoodShape GetCurrentShape() const { return CurrentShape; }
    UFUNCTION(BlueprintPure) float GetHealth() const { return Health; }
    // Shared material-agnostic structural interface used by stone/wood mixed structures.
    virtual EMaterialType GetMaterialType_Implementation() const override;
    virtual EStructuralNodeState GetStructuralState_Implementation() const override;
    virtual bool SetStructuralState_Implementation(EStructuralNodeState NewState) override;
    virtual float GetStructuralMass_Implementation() const override;
    virtual float GetStructuralStrength_Implementation() const override;
    virtual bool IsGrounded_Implementation() const override;
    UFUNCTION(BlueprintCallable) void SetHealth(float NewHealth);
    /** Optional canonical logistics segment this physical piece supports (e.g. a bridge plank). */
    UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, Category="Logistics") FName InfrastructureLinkKey = NAME_None;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> MeshComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<ULogStateMachine> StateMachine;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<ULogConnectionGraph> ConnectionGraph;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStructuralGraph> StructuralGraph;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<ULogPhysicsComponent> PhysicsComponent;
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Definition") TObjectPtr<ULogDefinition> Definition;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float Health = 100.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float BurnTimer = 0.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float CurrentLength = 65.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) EWoodShape CurrentShape = EWoodShape::Full;
    UFUNCTION() void HandleStateChanged(AActor* Log, ELogState NewState);
private:
    bool bStateDelegateBound = false;
    FVector BaseMeshScale = FVector::OneVector;
};
