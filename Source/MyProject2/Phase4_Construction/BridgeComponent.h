#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BridgeComponent.generated.h"
class ALogActor; class ULogDefinition;
UCLASS(ClassGroup=(Build),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UBridgeComponent:public UActorComponent
{GENERATED_BODY() public: UBridgeComponent(); UFUNCTION(BlueprintCallable) bool BuildBridge(const FVector& Start,const FVector& End,bool bNeedsSupport); UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxSpanWithoutSupport=600.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<ALogActor> LogClass; UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<ULogDefinition> DefaultDefinition; };
