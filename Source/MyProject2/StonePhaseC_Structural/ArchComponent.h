#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArchComponent.generated.h"
class AStoneActor;

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UArchComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UArchComponent();
    UFUNCTION(BlueprintCallable) TArray<AStoneActor*> BuildArch(const FVector& LeftBase, const FVector& RightBase, float Height, int32 VoussoirCount);
    UFUNCTION(BlueprintPure) bool IsArchStable(const TArray<AStoneActor*>& Arch) const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float VoussoirSpacing = 25.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float KeystoneAngleTolerance = 5.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AStoneActor> StoneClass;
};
