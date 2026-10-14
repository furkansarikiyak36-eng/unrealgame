#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LogBuoyancyComponent.generated.h"
UCLASS(ClassGroup=(Log),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API ULogBuoyancyComponent:public UActorComponent
{GENERATED_BODY() public: ULogBuoyancyComponent(); UPROPERTY(EditAnywhere,BlueprintReadWrite) float WaterLevelZ=0.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0.0")) float Buoyancy=0.6f; UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0.0")) float WaterDrag=3.0f; UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="1.0")) float ApproximateSubmergedDepth=50.0f; protected: virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override; };
