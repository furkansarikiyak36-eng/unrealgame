#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DemolitionComponent.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Structure),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UDemolitionComponent:public UActorComponent
{GENERATED_BODY() public: UDemolitionComponent(); UFUNCTION(BlueprintCallable) bool PartialDemolish(ALogActor* Log); UFUNCTION(BlueprintCallable) bool FullDemolish(ALogActor* Log); UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaterialRecoveryRate=1.0f; };
