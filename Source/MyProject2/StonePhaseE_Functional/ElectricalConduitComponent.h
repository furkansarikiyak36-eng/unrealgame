#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ElectricalConduitComponent.generated.h"
UCLASS(ClassGroup=(Electrical), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UElectricalConduitComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UElectricalConduitComponent();
    UFUNCTION(BlueprintCallable) bool ConnectCable(AActor* From, AActor* To);
    UFUNCTION(BlueprintCallable) bool DisconnectCable(AActor* Node);
    UFUNCTION(BlueprintCallable) bool AttachLight(AActor* Node);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxCableLength = 1500.0f;
    TArray<TWeakObjectPtr<AActor>> CableNodes;
};
