#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StickTypes.h"
#include "StickStackComponent.generated.h"
class AStickActor;
UCLASS(ClassGroup=(Stick), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStickStackComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStickStackComponent();
    UFUNCTION(BlueprintCallable) int32 AddToStack(AStickActor* Stick);
    UFUNCTION(BlueprintCallable) AStickActor* TakeFromStack();
    UFUNCTION(BlueprintCallable) int32 AddBundle(const FStickBundle& Bundle);
    UFUNCTION(BlueprintPure) int32 GetStackCount() const { return StackedSticks.Num(); }
    UFUNCTION(BlueprintPure) int32 GetMaxStack() const { return MaxStackSize; }
    UFUNCTION(BlueprintPure) bool IsFull() const { return StackedSticks.Num() >= MaxStackSize; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxStackSize = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName AttachSocket = TEXT("back_pack");
protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) TArray<TObjectPtr<AStickActor>> StackedSticks;
};
