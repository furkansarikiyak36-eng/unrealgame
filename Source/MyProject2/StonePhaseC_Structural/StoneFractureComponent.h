#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneFractureComponent.generated.h"
class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneFractureComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneFractureComponent();
    UFUNCTION(BlueprintCallable) bool Fracture(AStoneActor* Stone, const FVector& ImpactPoint);
    UFUNCTION(BlueprintPure) bool IsCracked(AStoneActor* Stone) const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MinPieces = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxPieces = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float CrackThreshold = 0.3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AStoneActor> RubbleClass;
};
