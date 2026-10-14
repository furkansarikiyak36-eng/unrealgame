#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldTypes.h"
#include "ChunkActivationComponent.generated.h"
class UChunkState; class UChunkActivationPolicy;
UCLASS(ClassGroup=(World),meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UChunkActivationComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UChunkActivationComponent();
 UFUNCTION(BlueprintCallable,Category="Activation") void ActivateChunk(UChunkState* Chunk);
 UFUNCTION(BlueprintCallable,Category="Activation") void DeactivateChunk(UChunkState* Chunk);
 UFUNCTION(BlueprintCallable,Category="Activation") void EvaluateActivation(UChunkState* Chunk,const FVector& PlayerLocation);
 UFUNCTION(BlueprintPure,Category="Activation") EChunkActivationMode GetCurrentMode() const{return CurrentMode;}
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UChunkActivationPolicy> Policy;
 UPROPERTY(BlueprintAssignable) FOnChunkActivated OnChunkActivated;
 UPROPERTY(BlueprintAssignable) FOnChunkDeactivated OnChunkDeactivated;
protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) EChunkActivationMode CurrentMode=EChunkActivationMode::Passive;
};
