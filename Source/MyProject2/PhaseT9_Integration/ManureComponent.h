#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ManureComponent.generated.h"
UCLASS(ClassGroup=(Farming),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UManureComponent:public UActorComponent{GENERATED_BODY()
public:
 UManureComponent();
 UFUNCTION(BlueprintCallable,Category="ManureComponent") float ComputeManureNitrogen(float NitrogenIntake,float NitrogenRetained,float NitrogenProducts) const;
 UFUNCTION(BlueprintCallable,Category="ManureComponent") void AccumulateManure(float Amount,float NitrogenContent);
 UFUNCTION(BlueprintCallable,Category="ManureComponent") float CollectManure(float Amount);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float ManureStorageMax=100.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float ManureAmount=0.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float ManureNitrogen=0.f;
};