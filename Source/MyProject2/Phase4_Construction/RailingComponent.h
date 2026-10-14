#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RailingComponent.generated.h"
class ALogActor; class ULogDefinition;
UCLASS(ClassGroup=(Build),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API URailingComponent:public UActorComponent
{GENERATED_BODY() public: URailingComponent(); UFUNCTION(BlueprintCallable) bool BuildRailing(const FVector& Start,const FVector& End,float Height=100.0f); UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<ALogActor> LogClass; UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<ULogDefinition> DefaultDefinition; };
