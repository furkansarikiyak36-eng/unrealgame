#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/StructuralMaterialTypes.h"
#include "StonePlacementComponent.generated.h"

class AStoneActor;
class USceneComponent;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStonePlacementComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStonePlacementComponent();
    UFUNCTION(BlueprintCallable) bool BeginPlacement(AStoneActor* Stone);
    UFUNCTION(BlueprintCallable) bool CancelPlacement();
    UFUNCTION(BlueprintCallable) bool UpdatePlacementTarget(FVector SurfacePoint, FVector SurfaceNormal);
    UFUNCTION(BlueprintPure) FTransform GetTargetTransform() const { return TargetTransform; }
    UFUNCTION(BlueprintPure) int32 GetPlacementOptionIndex() const { return PlacementOptionIndex; }
    UFUNCTION(BlueprintPure) AStoneActor* GetPreviewStone() const { return PreviewStone; }
    UFUNCTION(BlueprintPure) bool IsPlacing() const { return PreviewStone != nullptr; }
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
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) bool bHasPlacementTarget = false;
    FTransform OriginalActorTransform;
    EStructuralNodeState OriginalStructuralState = EStructuralNodeState::Loose;
    TWeakObjectPtr<USceneComponent> OriginalAttachComponent;
    FName OriginalAttachSocket = NAME_None;

    void ApplySelectedPlacementOption();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
