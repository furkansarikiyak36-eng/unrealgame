#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScaffoldComponent.generated.h"
class ALogActor; class ULogDefinition;
UCLASS(ClassGroup=(Build),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UScaffoldComponent:public UActorComponent
{GENERATED_BODY() public: UScaffoldComponent(); UFUNCTION(BlueprintCallable) bool BuildScaffold(const FVector& Base,float Height,float Width); UFUNCTION(BlueprintCallable) bool DemolishScaffold(); UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<ALogActor> LogClass; UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<ULogDefinition> DefaultDefinition; protected: UPROPERTY() TArray<TObjectPtr<ALogActor>> ScaffoldParts; };
