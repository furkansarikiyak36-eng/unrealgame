#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RoofFrameComponent.generated.h"
class ALogActor; class ULogDefinition;
UCLASS(ClassGroup=(Build),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API URoofFrameComponent:public UActorComponent
{GENERATED_BODY() public: URoofFrameComponent(); UFUNCTION(BlueprintCallable) bool BuildRidgeBeam(const FVector& Start,const FVector& End); UFUNCTION(BlueprintCallable) bool BuildPurlin(const FVector& Start,const FVector& End); UFUNCTION(BlueprintCallable) bool BuildRafter(const FVector& Start,const FVector& End); UFUNCTION(BlueprintCallable) bool CoverWithPlanks(const TArray<FVector>& Positions); UPROPERTY(EditAnywhere,BlueprintReadWrite) float PitchAngle=30.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<ALogActor> LogClass; UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<ULogDefinition> DefaultDefinition; };
