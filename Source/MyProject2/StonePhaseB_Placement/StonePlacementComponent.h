#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StonePlacementComponent.generated.h"

class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStonePlacementComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStonePlacementComponent();
    UFUNCTION(BlueprintCallable) void EnterAimMode();
    UFUNCTION(BlueprintCallable) void ExitAimMode();
    UFUNCTION(BlueprintCallable) void CycleFace();
    UFUNCTION(BlueprintCallable) void RotateStone(float Delta);
    UFUNCTION(BlueprintCallable) bool ConfirmPlacement();
    UFUNCTION(BlueprintCallable) bool UndoLastPlacement();
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ReachDistance = 250.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FaceSnapRadius = 25.0f;
protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) bool bAiming = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) TObjectPtr<AStoneActor> PreviewStone = nullptr;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) FTransform TargetTransform;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) TArray<TObjectPtr<AStoneActor>> PlacementHistory;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) int32 PlacementOptionIndex = INDEX_NONE;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) FVector TargetSurfaceNormal = FVector::UpVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) FVector TargetSurfacePoint = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) float ManualYawDegrees = 0.0f;

    void ApplySelectedPlacementOption();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
