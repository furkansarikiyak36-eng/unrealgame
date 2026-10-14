#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChunkManager.generated.h"
class UWorldStateSubsystem; class UChunkActivationPolicy;
UCLASS(ClassGroup=(World),meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UChunkManager : public UActorComponent
{
 GENERATED_BODY()
public:
 UChunkManager();
 UFUNCTION(BlueprintCallable,Category="Chunk") void UpdateChunkActivations(const FVector& PlayerLocation);
 UFUNCTION(BlueprintPure,Category="Chunk") int32 GetActiveChunkCount() const;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UChunkActivationPolicy> Policy;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float ActivationRadius=500.0f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bDebugDraw=false;
protected: virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
};
