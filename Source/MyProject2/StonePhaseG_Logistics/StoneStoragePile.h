#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoneStoragePile.generated.h"
class UStaticMeshComponent;
class AStoneActor;
UCLASS()
class MYPROJECT2_API AStoneStoragePile : public AActor
{
    GENERATED_BODY()
public:
    AStoneStoragePile();
    UFUNCTION(BlueprintCallable) bool Store(AStoneActor* Stone);
    UFUNCTION(BlueprintCallable) AStoneActor* Retrieve();
    UFUNCTION(BlueprintPure) int32 GetCount() const { return Stored.Num(); }
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PileMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Capacity = 100;
protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) TArray<TObjectPtr<AStoneActor>> Stored;
};
