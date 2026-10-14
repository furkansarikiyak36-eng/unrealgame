#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RaftActor.generated.h"
class UStaticMeshComponent; class ALogActor;
UCLASS() class MYPROJECT2_API ARaftActor:public AActor
{GENERATED_BODY() public: ARaftActor(); UFUNCTION(BlueprintCallable) bool AssembleFromLogs(const TArray<ALogActor*>& Logs); UFUNCTION(BlueprintCallable) void Steer(float Yaw); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RaftMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) float Buoyancy=1.0f; };
