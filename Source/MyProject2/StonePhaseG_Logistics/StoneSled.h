#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoneSled.generated.h"
class UStaticMeshComponent;
class AStoneActor;
UCLASS()
class MYPROJECT2_API AStoneSled : public AActor
{
    GENERATED_BODY()
public:
    AStoneSled();
    UFUNCTION(BlueprintCallable) bool LoadStone(AStoneActor* Stone);
    UFUNCTION(BlueprintCallable) void SetDragForce(float Force);
    UFUNCTION(BlueprintPure) int32 GetLoadCount() const { return Loaded.Num(); }
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Platform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Capacity = 60;
protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) TArray<TObjectPtr<AStoneActor>> Loaded;
};
