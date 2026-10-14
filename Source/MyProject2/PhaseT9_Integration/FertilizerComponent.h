#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FertilizerComponent.generated.h"
UCLASS(ClassGroup=(Integration),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UFertilizerComponent:public UActorComponent{GENERATED_BODY()
public: UFertilizerComponent(); UFUNCTION(BlueprintCallable,Category="Fertilizer") void ApplyFertilizer(FName Type,float Amount); UFUNCTION(BlueprintCallable,Category="Fertilizer") void ApplyManure(float Amount,float NitrogenContent); UPROPERTY(EditAnywhere,BlueprintReadWrite) float ManureUptakeEfficiency=.5f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float SyntheticUptakeEfficiency=.8f;};
