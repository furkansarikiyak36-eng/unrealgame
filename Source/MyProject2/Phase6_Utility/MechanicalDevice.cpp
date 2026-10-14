#include "Phase6_Utility/MechanicalDevice.h"
#include "Components/StaticMeshComponent.h"
AMechanicalDevice::AMechanicalDevice(){PrimaryActorTick.bCanEverTick=false;DeviceMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Device"));RootComponent=DeviceMesh;}
void AMechanicalDevice::Activate(){if(Power>0)bActive=true;} void AMechanicalDevice::Deactivate(){bActive=false;}
