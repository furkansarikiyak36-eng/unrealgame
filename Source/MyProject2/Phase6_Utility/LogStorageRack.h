#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LogStorageRack.generated.h"
class ALogActor; class UStaticMeshComponent;
UCLASS() class MYPROJECT2_API ALogStorageRack:public AActor
{GENERATED_BODY() public: ALogStorageRack(); UFUNCTION(BlueprintCallable) bool Store(ALogActor* Log); UFUNCTION(BlueprintCallable) ALogActor* Retrieve(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Frame; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Capacity=20; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TArray<TObjectPtr<ALogActor>> Stored; };
