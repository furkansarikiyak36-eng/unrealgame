// CompositeWallActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MaterialCore/MaterialTypes.h"
#include "CompositeWallActor.generated.h"

class UStaticMeshComponent;
class UCompositeStructureComponent;
class UMaterialInstanceStateComponent;

UCLASS()
class MYPROJECT2_API ACompositeWallActor : public AActor
{
    GENERATED_BODY()

public:
    ACompositeWallActor();

    UFUNCTION(BlueprintCallable, Category = "Wall")
    void InitializeDefaultWall();

    UFUNCTION(BlueprintCallable, Category = "Wall")
    void ApplyDamageToLayer(EStructureLayerRole LayerRole, float Amount, EMaterialReactionType Reaction);

    UFUNCTION(BlueprintCallable, Category = "Wall")
    void ApplyFireDamage(float Intensity, float DurationHours);

    UFUNCTION(BlueprintCallable, Category = "Wall")
    void ApplyWaterDamage(float Intensity, float DurationHours);

    UFUNCTION(BlueprintCallable, Category = "Wall")
    void ApplyImpactDamage(float Magnitude);

    UFUNCTION(BlueprintPure, Category = "Wall")
    bool IsStructurallyStable() const;

    UFUNCTION(BlueprintPure, Category = "Wall")
    EStructureLayerCondition GetOverallCondition() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> WallMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCompositeStructureComponent> StructureComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UMaterialInstanceStateComponent> InstanceState;

protected:
    virtual void BeginPlay() override;
    UFUNCTION() void HandleLayerConditionChanged(EStructureLayerRole LayerRole, EStructureLayerCondition NewCondition);
};
