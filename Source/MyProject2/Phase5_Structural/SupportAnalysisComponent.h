#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SupportAnalysisComponent.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Structure),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API USupportAnalysisComponent:public UActorComponent
{GENERATED_BODY() public: USupportAnalysisComponent(); UFUNCTION(BlueprintCallable) bool CanRemoveSupport(ALogActor* Support)const; UFUNCTION(BlueprintCallable) TArray<ALogActor*> GetOrphansIfRemoved(ALogActor* Support)const; UFUNCTION(BlueprintCallable) float ComputeLoad(ALogActor* Log)const; };
