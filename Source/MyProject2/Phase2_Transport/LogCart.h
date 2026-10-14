#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LogCart.generated.h"
class UStaticMeshComponent; class UBoxComponent; class ALogActor;
UCLASS() class MYPROJECT2_API ALogCart:public AActor
{GENERATED_BODY() public: ALogCart(); UFUNCTION(BlueprintCallable) bool LoadLog(ALogActor* Log); UFUNCTION(BlueprintCallable) bool UnloadLog(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body; UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> WheelL; UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> WheelR; UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> StorageVolume; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Capacity=6; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TArray<TObjectPtr<ALogActor>> LoadedLogs; };
