#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BridgeComponent.generated.h"
class ALogActor; class ULogDefinition;
UCLASS(ClassGroup=(Build),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UBridgeComponent:public UActorComponent
{GENERATED_BODY() public: UBridgeComponent(); UFUNCTION(BlueprintCallable) bool BuildBridge(const FVector& Start,const FVector& End,bool bNeedsSupport); UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxSpanWithoutSupport=600.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<ALogActor> LogClass; UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<ULogDefinition> DefaultDefinition; UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Logistics") FName LogisticsLinkKey=NAME_None; UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Logistics") FName LogisticsStartNodeKey=NAME_None; UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Logistics") FName LogisticsEndNodeKey=NAME_None; UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Logistics",meta=(ClampMin="0.1")) float LogisticsCapacityKg=1000.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Logistics",meta=(ClampMin="0.0",ClampMax="1.0")) float LogisticsRisk01=0.0f; virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override; };
