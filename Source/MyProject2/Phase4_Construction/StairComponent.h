#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StairComponent.generated.h"
class ALogActor; class ULogDefinition;
UCLASS(ClassGroup=(Build),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UStairComponent:public UActorComponent
{GENERATED_BODY() public: UStairComponent(); UFUNCTION(BlueprintCallable) bool BuildStair(const FVector& Base,const FVector& Top,int32 Steps); UPROPERTY(EditAnywhere,BlueprintReadWrite) float StepHeight=20.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float StepWidth=30.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<ALogActor> LogClass; UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<ULogDefinition> DefaultDefinition; };
