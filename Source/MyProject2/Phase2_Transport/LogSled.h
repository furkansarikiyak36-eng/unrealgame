#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LogSled.generated.h"
class UStaticMeshComponent; class ALogActor;
UCLASS() class MYPROJECT2_API ALogSled:public AActor
{GENERATED_BODY() public: ALogSled(); UFUNCTION(BlueprintCallable) bool LoadLog(ALogActor* Log); UFUNCTION(BlueprintCallable) void SetDragForce(float Force); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Platform; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Capacity=4; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TArray<TObjectPtr<ALogActor>> LoadedLogs; };
