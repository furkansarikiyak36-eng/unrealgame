#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MechanicalDevice.generated.h"
class UStaticMeshComponent;
UENUM(BlueprintType) enum class EDeviceType:uint8{WaterWheel,Windmill,LogSplitter};
UCLASS() class MYPROJECT2_API AMechanicalDevice:public AActor
{GENERATED_BODY() public: AMechanicalDevice(); UFUNCTION(BlueprintCallable) void Activate(); UFUNCTION(BlueprintCallable) void Deactivate(); UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> DeviceMesh; UPROPERTY(EditAnywhere,BlueprintReadWrite) EDeviceType DeviceType=EDeviceType::WaterWheel; UPROPERTY(EditAnywhere,BlueprintReadWrite) float Power=1.0f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bActive=false; };
