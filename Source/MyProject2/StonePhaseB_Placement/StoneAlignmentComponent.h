#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneAlignmentComponent.generated.h"
class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneAlignmentComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneAlignmentComponent();
    UFUNCTION(BlueprintCallable) FTransform ComputeAlignedTransform(AStoneActor* Moving, const FVector& DesiredLocation, const FVector& SurfaceNormal) const;
    UFUNCTION(BlueprintCallable) bool AlignToNeighbors(AStoneActor* Stone);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float AlignmentTolerance = 5.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float VerticalOffset = 0.0f;
};
