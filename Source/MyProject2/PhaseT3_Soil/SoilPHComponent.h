#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoilPHComponent.generated.h"
UCLASS(ClassGroup=(Soil),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API USoilPHComponent:public UActorComponent{GENERATED_BODY()
public: USoilPHComponent(); UFUNCTION(BlueprintPure,Category="PH") float ComputePHSuitability(float OptimalPH,float Tolerance)const; UFUNCTION(BlueprintCallable,Category="PH") void ApplyLime(float Amount); UFUNCTION(BlueprintCallable,Category="PH") void ApplySulfur(float Amount); UPROPERTY(EditAnywhere,BlueprintReadWrite) float LimeEffectPerUnit=.1f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float SulfurEffectPerUnit=-.1f;};
